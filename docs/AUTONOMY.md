# Autonomy Agency

I need to build a c++ class for implementing Autonomy for a game. The 
AutonomyAgent is to be conceived as the AI-like enterprise of the 
application when the game is in single-player mode. The main feature of 
the AutonomyAgent (AA for short so I don't have to keep typing it) is 
this: It has pre-initialization steps: 

1. Load and acquire the ECL ( 
Lisp ) DSL for determining cause and effect, testing predicates , and 
upon gathering input state (which I will discuss), render results 
(functional programming ideals leveraged) for use by the C++ host (AA). 

2. TBD 

3. upon the Running of the thread, the thread continues until a 
flag (TBD) is issued to indicate the end of the game, etc.. But the 
thread is considered a while(!done) affair. 

4. In the thread Runner, it 
descends through a strict Mealy State Machine (MSS). The internal state 
consists of the Slate of State only relevant to the AA and updated 
Slate of State that it gleans cyclically from the host software (via 
MySQL DB queries and other run-time state information per game session) 
-- details are academic. 

5. The evolution through the MSS is that every 
cycle it performs in order: 
    a) Gather Inputs 
    b) Perform Calculations 
    c) Render Output 
    d) Telemeter Results (special kind of render that is used 
to notify game-play, not update game-state or modify databases). 
    e) Wait for cyclical message pump**


## Gather Inputs 

Draw upon access to 
the database (which is modified by the other players' actions) and the 
overall game state owned by the Server, and also State held and last 
computed by the AA itself on the previous cycles. Calculations - this 
is where the AA meets the ECL / DSL. It's prescribed behavior based on 
Inputs gathered that yield new results and the results are two-fold: 
ONE: creation of Tasks that directly cause the "AI" to issue commands 
as if it was a player. (This plumbing is already done-- just treat this 
as an "inject" method that you will see in a moment.) Injection of 
commands follows precisely the same grammar parsing rules that the 
user-player follows. In the simplest terms, whatever the player enters 
for commands they are processed -- executed. The "AA" uses the exact 
same call-plumbing to do the same thing. Nothing is 'back-door'). 


## Calculations 

Calculations are complex perhaps because it needs to be tailored to 
achieve goals of the game and attempt to decision-tree its way to a 
solution based on the Inputs previously gathered. For you (here) to 
understand the scope of the goals, I'm going to attach the raw rules of 
the game for reference. 


## Rendering is the point where all of the 
results that were obtained in Calculate manifest in the actual use of 
inject() method to queue the commands into the game for execution. 
Commands by the AA that are injected share the same code path as 
commands from the user. This is important. It's not true that commands 
from the user-player are unilaterally blocked while the "AA" takes its 
turn. They cannot be. Although it is unlikely the "AA" will take 
wall-clock time exceeding a few seconds, it is still possible that the 
user-player can still enter commands that are ancillary to the overall 
game.. Query commands, game economy commands, inqueries about the user 
player's own state, score, etc.. However it is true that during either 
turn (user-player or "AA") there are two phases of the turn that are 
more or less secure and atomic. One is Movement. Ships (the things 
players have on the 'map') can be moved. Move ship A to hex1, move ship 
B to hex2, etc.. After movement is done, then the next phase a player 
(AA or user) is to pass through a Combat resolution phase. Combat is 
compulsary if at the end the active player's Move phase they have ships 
occuping a location that is also occupied by the enemy. When that 
happens (when that is detected AFTER movement phase is over) we are in 
combat and each player is responsible to carry out a combat-order, 
combat-commit, combat-assign-damage melee round-by-round per the rules. 
(combat-commit is a nuance for the computer software version of the 
game that allows the attacker or attackee to confirm their choices in 
their combat order. By order I mean, a declaration (by command) that 
"Ship W1 Attack W2 with-these-parameters" == that's an Combat Order. 
After each player during combat has issued (committed) orders, then if 
any damage is determined, each player self-assigns the damage to their 
own ships. 

The rules spell out what happens next, I won't repeat it.) 
But the handling of Input-gathering, Calculation, Rendering is repeated 
throughout each instance of the Combat Melee rounds.

### Example:
- Input gathered -- test if in combat state still; enemy ship(s) are still 
present in hex H1
- Calculate -- determine best mix of combat-order paramerters. 
- Rendering -- Inject commands into the parser.  Example, the "Combat Order command ..." 
- Telemetry -- managed far away from the Autonomy Agent.


## Telemeter 
To Telemeter is to convey results to the user-player -- rinse repeat. Other Rendering 
of commands (to cause them to be injected) could be things like 
commands to extract raw materials from star systems, refine materials, 
repair things, fabricate things, conduct trade (market simulation of 
the game) and so on. Telemetry - is just a last dash before the end of 
the cycle where results that are meaningful for the user-player are 
messaged across the UI. The best analogy is that the MSS is like a 
Pachinko machine, where the ball (the current state of the MSS) bops 
down through the pins past all of the stages of the MSS and then 
another flick of the handle (cyclic nature and/or pumped by messages) 
throws another ball at the top of the tableau again, and again. About 
messages: The UI is constructed to pump the server every 3 seconds 
(REST-ful interface) so there is a built in event pump to cause 
messaging to target the Thread of the AA. Then there is the other 
event-pump side effect of the user-player entering commands which 
causes the turn-handling to be able to poll the AA ("the state has 
obviously changed, player Joe is still active, but do you have commands 
that are not inhibited by rules to inject?"). "AA" is never shut out. 


Can you produce the rudimentary class that implements this. Don't drill 
into the Lisp-y parts yet (ECL) but be aware that we're using ECL. Stub 
out the calls.


