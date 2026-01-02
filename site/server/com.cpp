
void combat_prototype()
{

    if (cmd == "combat")
    {
        std::istringstream iss(cmdline);
        std::string cmdName;
        iss >> cmdName; // "combat"

        std::string action;
        iss >> action;
        if (action == "order")
        {
            // combat order <ship> [tactic=A|D|R] [target=ID] [d=N] [b=N] [s=N]
            // [t=N] [m=N]
            CombatOrder ord;
            ord.game_id = a.game_id;
            ord.round = 0;
            ord.tactic = 'A'; // Default to Attack
            // default target not needed
            // ord.target_id = "";
            ord.power_d = 0;
            ord.power_b = 0;
            ord.power_s = 0;
            ord.power_t = 0;
            ord.missiles_json = "[]";

            if (!(iss >> ord.ship_code))
            {
                resp->body = json_error("missing ship code");
                return;
            }
            ord.ship_code = upper_ascii(ord.ship_code);

            std::string token;
            while (iss >> token)
            {
                size_t eq = token.find('=');
                if (eq == std::string::npos)
                {
                    // check for implicit tactic (A, D, R)
                    if (token.size() == 1)
                    {
                        char c = std::toupper(token[0]);
                        if (c == 'A' || c == 'D' || c == 'R')
                        {
                            ord.tactic = c;
                            continue;
                        }
                    }
                    // Implicit target ID (e.g. "W1" in "combat order W2 D W1")
                    ord.target_id = token;
                    continue;
                }
                std::string key = to_lower(token.substr(0, eq));
                std::string val = token.substr(eq + 1);
                if (key == "tactic" || key == "mode" || key == "opt")
                {
                    if (!val.empty())
                        ord.tactic = std::toupper(val[0]);
                }
                else if (key == "target" || key == "tgt")
                {
                    ord.target_id = val;
                }
                else if (key == "d" || key == "drive")
                    ord.power_d = std::atoi(val.c_str());
                else if (key == "b" || key == "beam")
                    ord.power_b = std::atoi(val.c_str());
                else if (key == "s" || key == "screen")
                    ord.power_s = std::atoi(val.c_str());
                else if (key == "t" || key == "tube")
                    ord.power_t = std::atoi(val.c_str());
                else if (key == "m" || key == "missiles")
                    ord.missiles_json = val;
            }

            // No strict syntax check needed, defaults apply.

            std::string candidate_target(ord.target_id);
            Logger::instance().info(candidate_target);

            // Validation
            if (ord.tactic == 'D' && ord.target_id.empty())
            {
                eventText = "Combat order to dodge requires a target
                    opponent ship ";                   // Don't submit  } 
                    else
                {
                    CombatEngine ce(db, a.game_id);
                    eventText = ce.submit_order(owner, ord);
                }
            }
            else if (action == "resolve")
            {
                std::string hex;
                iss >> hex;
                if (hex.empty())
                {
                    resp->status = 400;
                    resp->body = json_error("missing hex");
                    Logger::instance().error(
                        "Trying to resolve combat, hex ID is missing");
                    return;
                }
                CombatEngine ce(db, a.game_id);
                eventText = ce.resolve_round(hex);
            }
            else if (action == "apply")
            { 
                // combat apply <ship> <attr = val>... std::string ship_code;
                if (!(iss >> ship_code))
                {
                    resp->body = json_error("missing ship code");
                    Logger::instance().error("Trying combat subcommand "
                                             "apply, ship code is missing");
                    return;
                }
                ship_code = upper_ascii(ship_code);
                std::map<std::string, int> assignments;
                std::string token;
                while (iss >> token)
                {
                    size_t eq = token.find('=');
                    if (eq == std::string::npos)
                        continue;
                    std::string k = to_lower(token.substr(0, eq));
                    int v = std::atoi(token.substr(eq + 1).c_str());
                    if (k == "beam" || k == "b")
                        k = "B";
                    else if (k == "d" || k == "drive" || k == "pd")
                        k = "D";
                    else if (k == "screen" || k == "s")
                        k = "S";
                    else if (k == "tube" || k == "t")
                        k = "T";
                    else if (k == "missiles" || k == "m")
                        k = "M";
                    assignments[k] = v;
                }
                CombatEngine ce(db, a.game_id);
                eventText = ce.apply_damage(owner, ship_code, assignments);
            }
        }
}
