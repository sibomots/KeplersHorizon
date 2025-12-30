#include "typedefs.h"
#include "services.h"

struct {
    Args args;
} Parameters;

static

void init(void)
{
    std::srand(static_cast<unsigned int>(std::time(NULL)));
}


void parse_args(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        std::string k = argv[i];
        auto next = [&](std::string &out)
        {
            if (i + 1 >= argc)
                throw std::runtime_error("missing arg for " + k);
            out = argv[++i];
        };
        if (k == "--dbhost")
            next(Parameters.args.dbhost);
        else if (k == "--dbuser")
            next(Parameters.args.dbuser);
        else if (k == "--dbpass")
            next(Parameters.args.dbpass);
        else if (k == "--dbname")
            next(Parameters.args.dbname);
        else if (k == "--listen")
            next(Parameters.args.listen);
        else if (k == "--port")
        {
            std::string t;
            next(t);
            Parameters.args.port = std::atoi(t.c_str());
        }
    }
    return a;
}


// 


void apply_arguments(int argc, char** argv)
{
    try
    {
        Parameters.args = parse_args(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "arg error: %s\n", e.what());
        return 2;
    }
}

        void banner() {
        std::fprintf(stderr,
                     "[%s] Kepler's Horizon_server listening on %s:%d\n",
                     now_iso().c_str(), args.listen.c_str(), args.port);
        Logger::instance().info(std::string("Server starting. Build SHA: ") +
                                GIT_SHA);
        }
    load_db()
    {
    try
    {
        Db db;
        db.connect(args.dbhost, args.dbuser, args.dbpass, args.dbname);

        int srv = ::socket(AF_INET, SOCK_STREAM, 0);
        if (srv < 0)
            throw std::runtime_error("socket failed");

        int one = 1;
        setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(args.port));
        addr.sin_addr.s_addr = inet_addr(args.listen.c_str());

        if (::bind(srv, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            throw std::runtime_error(std::string("bind failed: ") +
                                     std::strerror(errno));
        }
        if (::listen(srv, 16) < 0)
        {
            throw std::runtime_error("listen failed");
        }

        while (true)
        {
            sockaddr_in cli;
            socklen_t clen = sizeof(cli);
            int fd = ::accept(srv, (sockaddr *)&cli, &clen);
            if (fd < 0)
                continue;

            HttpRequest req = http_parse(fd);
            HttpResponse resp;

            try
            {
                dispatch_request((const HttpRequest *)&req, &db,
                                 (HttpResponse *)&resp);
            }
            catch (const std::exception &e)
            {
                resp.status = 500;
                resp.body =
                    json_error(std::string("server error: ") + e.what());
            }

            std::string out = http_serialize(resp);
            // Logger::instance().debug(out.c_str());
            ::send(fd, out.c_str(), out.size(), 0);
            ::close(fd);
        }
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }
    }
