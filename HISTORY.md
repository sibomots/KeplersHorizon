# Origins and History

This project has its roots in the early 1980s, during a time when personal 
computers were uncommon enough to feel slightly magical, and when games, 
programming, and science fiction tended to blur together rather than sit in 
neat categories. In 1984, shortly after the Macintosh was introduced, a 
neighbor acquired one of the early models. He was an architect, and the 
computer was meant for professional use—drawing, layout, and experimenting 
with Apple’s new graphical interface.

I was fifteen years old, and I did not own a computer at the time.

Instead, I would go over to his house after school, often most afternoons, 
and he would let me use the Macintosh to write programs. There was nothing 
formal about the arrangement. I showed up with notebooks, loose paper, and 
ideas, and I worked while the computer was available. When it wasn’t, I 
thought about what I would try next time. The Mac stayed where it was, and I 
adapted to its availability rather than the other way around.

Microsoft BASIC for the Macintosh had just become available, and it was 
approachable in a way that mattered. It allowed experimentation without 
requiring a deep understanding of the operating system, which was important 
when computer time was borrowed and mistakes were expensive. BASIC let you 
try things, break them, and try again without ceremony.

At the same time, programming was only one part of the picture.

There was a local hobby store in the neighborhood called **U.S.S. 
Enterprise**, and it played a quiet but important role. It sold role-playing 
games, tabletop wargames, dice, books, lead miniatures, and boxed games that 
tended to vanish once production runs ended. Dungeons & Dragons, Gamma World, 
Traveller, and various hex-and-counter wargames all shared the same shelves, 
and it was entirely normal to move between them without thinking of them as 
separate hobbies.

Among those games was **WarpWar**, originally published in 1977. I owned 
WarpWar at the time and played it alongside everything else.

WarpWar was austere in a way that took time to appreciate. Space was not a 
continuous plane but a network of systems connected by warplines. Movement, 
reach, and position mattered more than spectacle, and the rules existed to 
force decisions rather than to decorate theme. I did not analyze it in those 
terms at the time, but its assumptions settled in and stayed there.

There was also another influence, less tangible but persistent. Somewhere 
during that same period, I had access—intermittent and indirect—to a 
multi-user system running the OASIS operating system. Terminals were 
distributed throughout a facility and connected via serial ports, and among 
the programs available on that system was a text-based space game.

What stood out about that game was not combat, but economics. Trade, 
production, and markets were treated as central mechanics rather than 
background flavor. Systems mattered differently depending on what they 
produced or consumed, and decisions accumulated consequences over time. It 
felt less like a puzzle and more like a world that responded to pressure.

The name of that game, along with its authorship and implementation details, 
has been lost. No screen-shots survive, and no source code remains. This was 
long before the habit of archiving everything digitally. What remained was 
the impression that a space game could be about structure rather than 
spectacle, and that simulation did not need to announce itself loudly to be 
effective.

The game written on the borrowed Macintosh was called **Borealis**. The name 
came from the constellation *Corona Borealis*, chosen mostly because it 
sounded astronomical without being dramatic. It felt appropriate, and that 
was enough.

Borealis was not intended as a reproduction of WarpWar, nor as a direct 
descendant of any single game. Instead, it sat somewhere between influences. 
It borrowed the structural clarity of WarpWar, the exploratory framework of 
*Super Star Trek*–style games, and the richer economic ideas suggested by 
the OASIS system game. Markets existed. Resources had context. Movement 
across space required planning rather than simple traversal. Combat was 
present, but it was not the only thing happening.

The constraints were real and unavoidable. Programming time was limited to 
after-school hours, the computer was not always available, storage was 
limited, and debugging involved print statements, guesswork, and patience. As 
a result, the code grew large enough to become awkward. Sections were 
rewritten repeatedly, features were added and removed, and balance was 
improvised. Documentation was minimal. None of this was unusual.

In 1985, *Macworld* magazine and Microsoft announced a BASIC programming 
contest. Programs written in Microsoft BASIC for the Macintosh were invited, 
and games and simulations were explicitly permitted under the Entertainment 
category. Submissions were to be mailed on physical disks.


[Borealis was submitted.](http://spin-off.com/contest-page-MacWorld_8503_March_1985.pdf)


No confirmation arrived, and no feedback followed. According to the contest 
rules, this was expected. All entries became the property of Microsoft, and 
only winners were notified. The disk was mailed, and whatever happened to it 
afterward is unknown. That was the end of Borealis as an active project.

Not long after that period, I eventually bought my first computer of my own: 
a Macintosh Plus, in 1986. By then the circumstances were different. Access 
was no longer borrowed, time was no longer rationed, and programming no 
longer had to fit into someone else’s schedule. The original Borealis did 
not resume development, but the shift mattered. It marked the point where 
computing stopped being something visited after school and became something 
that could be lived with continuously.

In the years that followed, my professional work moved into real-time and 
safety-critical software, particularly in aerospace. The standards and 
expectations of that work were necessarily rigorous. Code needed to be 
correct, deterministic, testable, and maintainable. Looking back, the 
original Borealis was extremely rudimentary. That is not a criticism; it is 
simply a fact. It was written under severe constraints by someone still 
learning what software could be. The ideas were larger than the 
implementation.

Those ideas went dormant for a long time.

They resurfaced during a holiday visit from my college-age son. WarpWar came 
off the shelf, and we played it together. What stood out was not nostalgia, 
but clarity. WarpWar remains an elegant design, one that models space as a 
network and forces players to think in terms of reach, logistics, and 
commitment. After playing, a familiar thought surfaced: this could be turned 
into something.

The goal was not to recreate Borealis, nor to modernize WarpWar directly, but 
to build something playable that combined WarpWar’s structural discipline 
with a much richer economic and strategic milieu. There was also a practical 
constraint. I wanted to write the game quickly, while my son was still home, 
so that we could actually play it together.

To do that, I used AI-assisted tools.

Several AI agents were prompted to draft the initial skeleton of the game. 
The design was straightforward: a web-based, two-player game with a REST-ful 
server written in C++. I described the kinds of routines needed, the data 
flows, and the responsibilities of each component, and the agents produced 
code.

None of it worked.

That was expected. The value was not in correctness, but in structure. The 
generated code provided scaffolding: function boundaries, rough control flow, 
and data models that could be shaped and corrected. From there, the work 
became familiar. Code was revised, bugs were fixed, regressions reappeared, 
assumptions were corrected, and interfaces were clarified.

This process was not meaningfully different from earlier eras of software 
development, except in speed and surface area. No one was once ashamed to use 
manuals, then Usenet, then Yahoo, then Google, then Stack Overflow. Using AI 
to assist with the more pedantic parts of software construction follows the 
same pattern. It allows attention to be focused where it matters: gameplay, 
flow, and system behavior.

The result is what might reasonably be called **Borealis 2.0**.

It is not finished, and it is not perfect, but it exists. My son and I can 
play it over the web. The systems interact, the universe behaves, and the 
game reflects, in a more complete way, the ideas that motivated the original. 
There are already clear ideas for improvement, and there always will be, but 
this version matters because it is playable.

What began on a borrowed Macintosh has become something that can be shared, 
extended, and played. The universe is larger, the tools are better, and the 
questions remain the same.

And this time, the computer is not borrowed.

