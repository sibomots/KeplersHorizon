USE khdb;
DELETE FROM users;
INSERT INTO users (username, password_plain)
VALUES
('kirk', 'bones'),
('spock', 'indeed'),
('khan', 'bummer'),
('han','odds'),
('bill','whoa'),
('ted','dude'),
('rocsorc','rocks'),
('sibomots','heynow');
select * from users;
