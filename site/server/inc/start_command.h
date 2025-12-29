#ifndef __START_COMMAND_H__
#define __START_COMMAND_H__

#include <memory>
#include "icmd.h"
#include "typedefs.h"

class StartCommand : public ICmd
{
private:
    ScenarioType m_scenario;

public:

    class Builder {
         public:
           ScenarioType _scenario = ScenarioType::UNDEFINED;

           Builder& set_scenario(ScenarioType typ) {
                _scenario = std::move(typ);
                return *this;
           }

           // Returns unique_ptr to base interface ICmd
           std::unique_ptr<ICmd> build() {
                // Accessing private constructor of StartCommand
                // Cannot use make_unique with private constructor easily without friend
                // specialized handling, so sticking to new wrapped in unique_ptr
                // or just define builder inside class (which it is).
                return std::unique_ptr<StartCommand>(new StartCommand(_scenario));
           }
    };
   
private:
    StartCommand(ScenarioType scenario) :
          m_scenario(scenario)
    {}

public:
    // ICmd interface to invoke what must occur when the
    // command is executed. ONLY this class knows what to do
    // when the operation 'to start the game' means.
    //
    // Anything that it needs to know should be built (builder)
    // into the class.  It can learn facts from either data
    // 'built' into it (builder pattern) -- or learn facts
    // from queries against the StateMachine or learn facts
    // from querying the database.

    // As a result of invoke, the game has started
    // the first phase for the player with initiative is
    // the state in the StateMachine.  This method (invoke)
    // and this class StartGame, and any other Command class
    // that impleemnts ICmd does modify the State of the 
    // game.. Only the  StateMachine is in control of the State

    virtual bool invoke(void);

    // The way this is used:
    //
    // During the parsing of user commands from the terminal
    // the nouns and verbs that relate to this command are learned.

    // For instance the parsing detects the command:
    //   start 
    // then it detects the noun
    //   learning or basic or advanced -- the scenario
    //
    // Verb = start
    // Noun = <the scenario>

    // Then when the Verb + Noun(s) are detected by parsing
    // the command is built:
    //
    // std::unique_ptr<ICmd> pCmd = StartCommand::Builder()
    //                                  .set_scenario(ScenarioType::BASIC)
    //                                  .build();
    //
    // Then the command (the interface to the command is pCmd)
    // is given to something that invokes it: 
    //
    //    pCmd->invoke(); 
    //    // or passed to StateMachine:
    //    StateMachine::getInstance().active_player_execute(pCmd.get());
    //
    // The unique_ptr automatically handles cleanup when it goes out of scope.
    //

    // (NOTE: Let the Telemetry figure out how to escape or not escape
    // certain lines or parts of the message.. In fact the best
    // thing is if the Telemetry::write method just takes plain 
    // vector of strings.. Escape the contents of the strings for
    // JSON, but DO NOT do json_esc for the newline characters
    // between strings.)
    //
    // Further it is assumed that when it emits output
    // then it does so via the Telemetry::write(T msg)
    // where T is really std::vector<std::string>
    //
    // with the full intent
    // that the output is destinated for the UI of the players
    // And any output that is intended to be destinated for the
    // console of the server (REST server), is sent to the Logger::
    //
    // This class, objects of this class or any command
    // should not know how to cause text to be rendered to the "user"
    // or the console.
    // 
};

#endif
