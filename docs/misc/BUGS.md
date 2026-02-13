## Bugs

A software package without reported bugs is software that isn't used -- and used fully.

Software development expects bugs reported.

Bring the bugs!

[The Bug/Issue Database](https://github.com/sibomots/KeplersHorizon/issues)

* When filing bugs, please use the Issue tracker (link above)
* Use the LABELS that are provided.  Add as many labels to refine the category of the bug.
* Make sure the bug has the right person assigned to it. If you are unsure, leave it blank (don't guess).
* Anything in Issues (any bugs submitted) that do not directly relate to the files within
the repository or do not relate to the game presented here will be Closed.

## How to Report Bugs

Not everyone filing a bug is a software engineers.  Software engineers get a little
prickly when bad bugs are filed because it just means more back-and-forth questions
to understand what the real issue is.   (*If only they wrote a good bug, I could have
understood the problem and fixed it quicker*, etc..)

This is the nature of user-oriented software -- People need a little help writing good
bugs for those who must resolve them.

So for that case, here is a Primer:


### Primer --  What is a Bug?

A bug is these things:

1. **A bug is a statement of the problem, issue, or defect** (or even a question). Sometimes a Bug is merely just a task to  perform.  A feature to add, more capability to provide that was missing
from the software.  Development of the software involves traversing the  road-map of 
features planned and desired.  Those efforts are encapsulated as bugs also.  (2-4 sentences)

Most importantly, a Bug is related (linked) to something that was required.  In other-words
a bug report of a defective behavior that was never planned, offered, or advertised by
the design of the software is less a bug and more a feature request.   In this
repository, we're not going *Full On IEEE System Engineering Mode* -- so things are relaxed.

Just report your bug, whatever you think it is.

2. **A bug is reproducible** A brief list of stimulus steps to reproduce evidence of the bug.  If warranted, provide the steps taken from initial state that causes the bug to be evident.  Example:

- Logged in
- On my turn I used the command 'XX'
- I found the error message in error because 'XX' should generate YY

Exceptions -- *Some bugs just cannot be reproduced* so the next best thing is to start with a careful review of all of the stimulus that occurred PRIOR to the bug exhibiting the defective behavior.

3. Expected Results (2-3 sentences if possible).

**A Bug includes a list of what you expected to happen.**   Often, some bugs are not really
defects because of a misunderstanding of what the software was supposed to do given the
stimulus.

Or maybe not.  You might have been correct to conclude the expected behavior is not matched
by reality.  This is a bug to report.

Example:

<i>
Upon login, issuing the command 'XX' should reveal the 'YY' because the rules and
documentation state this is the behavior of the software.
</i>

4. Actual Results (2-3 sentences if possible). Example:

**A bug must include the actual results.**

If we don't know what actually happened, we are challenged to figure out what to fix.

Instead, when the command 'XX' was issued, the system produced an error message: 
*Dave's not here*.

5. **Optionally, a Bug can provide  ideas and possible solutions.**

If there are suggestions for improved behavior,
or suggestions for actual patches to the  software, documentation, etc.. then mention them here.  Further, if inclined:

- Fork the repo.
- Test your changes on a NEW branch on YOUR fork.
- Submit a PULL REQUEST of your branch into the main branch of THIS repo.
- Await further instructions.


## About Pull Requests

All Pull-Requests should describe briefly what they offer.

Example description that is IN THE PULL REQUEST DESCRIPTION:


<i>
This pull request will change files foo.cpp, bar.h and baz.cpp.
The impact of the change is that functions for handling XX to produce YY
are correctly routed to the appropriate Flux Capacitor and the effect is
aligned with the stated rules and behavioral description of the game.

The fix was tested on a branch with this set of stimulus:

Etc..
</i>


