#!/usr/bin/env perl
use strict;
use warnings;

# Generate site/web/map_view.html from DB contents (Option 1: DB is source of truth).
#
# This script intentionally does NOT use DBI/DBD::mysql to avoid CPAN dependencies.
# It shells out to the local `mysql` client.
#
# Environment variables (all optional):
#   KH_DB_HOST   (default: localhost)
#   KH_DB_USER   (default: root)
#   KH_DB_PASS   (default: empty)
#   KH_DB_NAME   (default: khdb)
#   KH_GAME_ID   (default: 1)
#   KH_TEMPLATE  (default: site/web/map_view.template.html)
#   KH_OUTPUT    (default: site/web/map_view.html)

my $host = $ENV{KH_DB_HOST} // 'localhost';
my $user = $ENV{KH_DB_USER} // 'root';
my $pass = $ENV{KH_DB_PASS} // '';
my $db   = $ENV{KH_DB_NAME} // 'khdb';
my $gid  = $ENV{KH_GAME_ID} // '1';

my $template_path = $ENV{KH_TEMPLATE} // 'site/web/map_view.template.html';
my $output_path   = $ENV{KH_OUTPUT}   // 'site/web/map_view.html';

sub slurp {
  my ($path) = @_;
  open(my $fh, '<', $path) or die "Unable to open $path: $!";
  local $/;
  my $s = <$fh>;
  close($fh);
  return $s;
}

sub spew {
  my ($path, $content) = @_;
  open(my $fh, '>', $path) or die "Unable to write $path: $!";
  print $fh $content;
  close($fh);
}

sub mysql_cmd {
  my ($sql) = @_;
  my @cmd = (
    'mysql',
    '--batch',
    '--raw',
    '--skip-column-names',
    '-h', $host,
    '-u', $user,
  );
  if (defined($pass) && length($pass)) {
    push @cmd, "-p$pass"; # note: visible in process list; acceptable for local build tooling.
  }
  push @cmd, '-D', $db, '-e', $sql;

  # Capture output
  my $out = '';
  {
    open(my $ph, '-|', @cmd) or die "Failed to run mysql: $!";
    local $/;
    $out = <$ph> // '';
    close($ph);
    my $ec = $? >> 8;
    if ($ec != 0) {
      die "mysql exited with status $ec";
    }
  }
  return $out;
}

sub json_escape {
  my ($s) = @_;
  $s =~ s/\\/\\\\/g;
  $s =~ s/\"/\\\"/g;
  $s =~ s/\n/\\n/g;
  $s =~ s/\r/\\r/g;
  $s =~ s/\t/\\t/g;
  return $s;
}

sub trim {
  my ($s) = @_;
  $s =~ s/^\s+//;
  $s =~ s/\s+$//;
  return $s;
}

# Query star systems
my $sys_sql = "SELECT hex_id, name, is_base FROM star_systems WHERE game_id=" . int($gid) . " ORDER BY name";
my $sys_out = mysql_cmd($sys_sql);

my @systems;
for my $line (split(/\n/, $sys_out)) {
  next if $line !~ /\S/;
  my ($hex, $name, $is_base) = split(/\t/, $line);
  $hex  = trim($hex // '');
  $name = trim($name // '');
  $is_base = trim($is_base // '0');
  next if $hex eq '' || $name eq '';

  my $type = ($is_base eq '1') ? 'base' : 'star';
  push @systems, { hex => $hex, type => $type, name => $name };
}

die "No star systems found in DB (star_systems is empty for game_id=$gid)" if scalar(@systems) == 0;

# Query warplines
my $wl_sql = "SELECT a_hex, b_hex FROM warplines WHERE game_id=" . int($gid) . " ORDER BY id";
my $wl_out = mysql_cmd($wl_sql);

my @warplines;
for my $line (split(/\n/, $wl_out)) {
  next if $line !~ /\S/;
  my ($a, $b) = split(/\t/, $line);
  $a = trim($a // '');
  $b = trim($b // '');
  next if $a eq '' || $b eq '';
  push @warplines, [$a, $b];
}

# Emit JS literals
my $star_json = "[\n";
for my $i (0 .. $#systems) {
  my $s = $systems[$i];
  my $hex  = json_escape($s->{hex});
  my $type = json_escape($s->{type});
  my $name = json_escape($s->{name});
  $star_json .= "    { hex: \"$hex\", type: \"$type\", name: \"$name\" }";
  $star_json .= "," if $i < $#systems;
  $star_json .= "\n";
}
$star_json .= "  ]";

my $wl_json = "[\n";
for my $i (0 .. $#warplines) {
  my ($a, $b) = @{$warplines[$i]};
  $a = json_escape($a);
  $b = json_escape($b);
  $wl_json .= "    [\"$a\", \"$b\"]";
  $wl_json .= "," if $i < $#warplines;
  $wl_json .= "\n";
}
$wl_json .= "  ]";

my $tmpl = slurp($template_path);

$tmpl =~ s/__STAR_HEXES__/$star_json/g;
$tmpl =~ s/__WARPLINES__/$wl_json/g;

spew($output_path, $tmpl);

print "Generated $output_path from DB (game_id=$gid)\n";
