#include <VTop.h>

#include <iostream>

#include <readline/history.h>
#include <readline/readline.h>

#include "sdb.hpp"
#include "dut_proxy.hpp"

SDBState sdb_state;
int sdb_halt_ret;

static char* rl_gets()
{
    static char* line_read = nullptr;

    if (line_read)
    {
        free(line_read);
        line_read = nullptr;
    }

    line_read = readline(">> ");

    if (line_read && *line_read)
        add_history(line_read);

    // default to use the last command
    if (line_read && line_read[0] == '\0')
    {
        HIST_ENTRY* prev = previous_history();
        if (prev)
            return prev->line;
    }

    return line_read;
}

static int cmd_c(char* args)
{
    cpu_exec(-1);
    return 0;
}

static int cmd_q(char* args)
{
    sdb_state = SDBState::Quit;
    return -1;
}

// si [N]
static int cmd_si(char* args)
{
    if (!args)
    {
        cpu_exec(1);
        return 0;
    }

    char* endptr;
    uint64_t n = strtol(args, &endptr, 10);
    if (endptr == args)
    {
        printf("si: Expected a number.\n");
        return 0;
    }
    cpu_exec(n);
    return 0;
}


// info [r/w]
static int cmd_info(char* args)
{
    if (strcmp(args, "r") == 0)
        isa_reg_display();
    else if (strcmp(args, "w") == 0)
        wp_display();
    else
    {
        printf("info: Unknown subcommand '%s'\n", args);
    }
    return 0;
}

// x [n] [EXPR]
static int cmd_x(char* args)
{
    if (!args)
    {
        printf("x: Expected two arguments.\n");
        return 0;
    }

    char* endptr;
    uint64_t n = strtol(args, &endptr, 10);

    if (args == endptr)
    {
        printf("x: Expected a number.\n");
        return 0;
    }

    bool success;
    word_t res = expr(endptr, &success);

    if (!success)
    {
        printf("x: Failed to evaluate expression.\n");
        return 0;
    }

    if (res % 4 != 0)
    {
        printf("x: Address must be aligned to 4 bytes.\n");
        return 0;
    }

    auto& mem = sim_handle.get_memory();

    if (!mem.in_bound(res) || !mem.in_bound(res + 4 * n))
    {
        printf("x: Address out of range.\n");
        return 0;
    }

    for (word_t i = 0; i < n; i++)
        printf("0x%x: 0x%08x\n", res + i * 4, mem.read(res + i * 4));

    return 0;
}

static int cmd_p(char* args)
{
    if (args == nullptr)
    {
        printf("p: Expected an expression.\n");
        return 0;
    }

    bool success;
    word_t res = expr(args, &success);

    if (!success)
    {
        printf("p: Failed to evaluate expression.\n");
        return 0;
    }

    printf("0x%x\n", res);

    return 0;
}

void wp_create(char* expr);
// w [EXPR]
static int cmd_w(char* args)
{
    if (args == nullptr)
    {
        printf("w: Expected an expression.\n");
        return 0;
    }

    wp_create(args);

    return 0;
}

void wp_delete(int n);
// d [N]
static int cmd_d(char* args)
{
    if (args == nullptr)
    {
        printf("d: Expected an index.\n");
        return 0;
    }

    char* endptr;
    int n = (int)strtoll(args, &endptr, 10);
    if (endptr == args)
    {
        printf("d: Expected a number.\n");
        return 0;
    }

    wp_delete(n);

    return 0;
}

static int cmd_help(char* args);

static struct
{
    const char* name;
    const char* description;
    int (*handler)(char*);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    {
        "si",
        "Execute N instructions one by one and then pause. If N is omitted, the "
        "default is 1.",
        cmd_si
    },
    {
        "info", "Print register status(r) or watchpoint information(w).",
        cmd_info
    },
    {
        "x", "Display N consecutive 4-byte words in hexadecimal at given address.",
        cmd_x
    },
    {"p", "Evaluate the expression.", cmd_p},
    {"w", "Pause execution when the value of the expression changes.", cmd_w},
    {"d", "Delete the watchpoint with index N.", cmd_d}
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char* args)
{
    /* extract the first argument */
    char* arg = strtok(nullptr, " ");
    int i;

    if (arg == nullptr)
    {
        /* no argument given */
        for (i = 0; i < NR_CMD; i++)
        {
            printf("%-4s - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    }
    else
    {
        for (i = 0; i < NR_CMD; i++)
        {
            if (strcmp(arg, cmd_table[i].name) == 0)
            {
                printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
                return 0;
            }
        }
        printf("Unknown command '%s'\n", arg);
    }
    return 0;
}

void sdb_mainloop()
{
    for (char* str; (str = rl_gets()) != nullptr;)
    {
        char* str_end = str + strlen(str);

        /* extract the first token as the command */
        char* cmd = strtok(str, " ");
        if (cmd == nullptr)
        {
            continue;
        }

        /* treat the remaining string as the arguments,
         * which may need further parsing
         */
        char* args = cmd + strlen(cmd) + 1;
        if (args >= str_end)
        {
            args = nullptr;
        }

#ifdef CONFIG_DEVICE
        extern void sdl_clear_event_queue();
        sdl_clear_event_queue();
#endif

        int i;
        for (i = 0; i < NR_CMD; i++)
        {
            if (strcmp(cmd, cmd_table[i].name) == 0)
            {
                if (cmd_table[i].handler(args) < 0)
                {
                    return;
                }
                break;
            }
        }

        if (i == NR_CMD)
        {
            printf("Unknown command '%s'\n", cmd);
        }
    }
}

int is_exit_status_bad()
{
    int good = (sdb_state == SDBState::End && sdb_halt_ret == 0) || (sdb_state == SDBState::Quit);
    return !good;
}

int main(int argc, char* argv[])
{
    init_regex();
    init_wp_pool();

    sdb_state = SDBState::Stop;

    // INIT
    if (argc != 2 && argc != 3)
    {
        printf("Usage: %s [filename] [ELF (optional)]\n", argv[0]);
        return -1;
    }

    sim_handle.init_sim(argv[1]);
    sim_handle.reset(10);

    if (argc == 3)
    {
        IFDEF(CONFIG_FTRACE, init_ftrace(argv[2]));
    }

    sdb_mainloop();

    sim_handle.cleanup();
    return is_exit_status_bad();
}
