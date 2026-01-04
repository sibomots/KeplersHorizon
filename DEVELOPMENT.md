# Developing

Software development for Kelpler's Horizon involves three distinct domains of
experience and production.

1.  C++ - The REST server is written in C++17 and likely will remain on that
    version.
2.  SQL - The back-end database that the software supports is MySQL (for the time
    being) and all of the queries and operations for the database are  embedded
    SQL statements invoked by the REST server.  There are a few *do-once* operations
    involved for setting up the database before the server runs.  
3.  JavaScript/CSS/HTML - The front-end user-facing software is primarily HTML/CSS
    tailored content with JavaScript used to conduct REST transactions with
    the server.  The goal is to minimize to practically zero any dynamic behavior
    within JavaScript for the UI.  Instead, REST is used by the server and client
    to wash the UI handling JavaScript with updated messages to alter the content
    of UI elements.

## Other Core Technologies

`lex` and `yacc` (or their GNU equivalents) were the basis for the user-command
processing engine.

This proved to be a far better way to construct the *grammar* of the command
language and augment the command-suite with new commands, handle command
invocation, and so on.

## Design Patterns

There are two main design patterns used.  

1. Singleton
2. Builder

In the server there the phrase *there can be only one* is a phrase that means
in effect we want to isolate and unify behaviors preventing other software Agencies
and Actors to instantiate them.

* Logger
* DatabaseManager
* StateMachine
* Telemetry

Are some examples of the classes (collections of data and behavior) which are
probably better off Singletons.

*Builder* pattern was used to abstract the notion of what a Command is and
so for each command that can be offered to the system by the user, it is 
contained as a class, *built* with Builder pattern, then *invoked.*

There might be a slightly better design pattern or other patterns to use
if a major refactor of the software is ever done.

# AI-Assisted

Let me get this out of the way right now.

**The software that AI generates is for the most part pure crap.**

Yes, it can write it quickly and yes, sometimes it can generate a useful idiom
that can rapidly get a prototype to work.

But for the most part -- this is **NOT HOW I WRITE SOFTWARE**.

I have tried many times to train this AI to do things a certain way, and still
the AI has only so much retention capabilities.   Rules or Directives that
*all of your software shall not pre-initialize std::string to "" since that
is already the default behavior of that class*

It's tedious and sometimes disappointing that behind all of this advanced AI, there
is still a 12-year old level of software engineering skill emerging too often.

I don't personally write code this pedantic.

Now.. Onto my point:

The upside is that a lot of the software was developed with the aid of AI.

Most of the software was co-developed with AI, specifically the **Opus-4.5** 
model and the AI Agent **Antigravity**.  This was a workable solution because
a lot of the details of the software are somewhat repetitive.  

Also, the quickness of the AI to generate the SQL and organize the Schema
of the database was very effective.

The downside is that a lot of the software was developed with the aid of AI.
And, this means the quality and effectivity of the software is in question.

AI Models of any kind are going to make many bad decisions about how to handle
data, how to compose data structures, and flow of execution.  AI models are 
terribly myopic when it comes to the architecture of the software -- they often
will focus on narrow and tight examples and fail to see the broader implications
in the architecture and design.

In plain English, AI models have no qualms about using strings when integers are
more suitable for basic data types, database fields and so on.  This leads to
a proliferation of software that is comparing characters and strings when simple
numeric (integer) based enumerations, flags, and fields are far easier and 
simpler to manage -- and debug.


# More, Later.

TBD


# Development and Testing

Some odds and ends for helping to test the software

(This is a note section that will be rearranged later)


## Screen

A great tool for running the server, of course.

An example usage:

```
# Start a named screen session for a python script
screen -S long_job -dm python my_app.py

# Check if it's running (optional)
screen -ls

# Detach (if you started it interactively)
Ctrl-a d

# Re-attach later to see its output
screen -r long_job
```

