// Written by Alexis Wilke based on
// https://en.wikipedia.org/wiki/Little_man_computer
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Source: 
// https://github.com/AlexisWilke/little-man-computer


#include    <cstring>
#include    <fstream>
#include    <iomanip>
#include    <iostream>
#include    <map>
#include    <string>
#include    <vector>


typedef int mnemonic_t;

constexpr mnemonic_t    MNEMONIC_NONE = -1;
constexpr mnemonic_t    MNEMONIC_HLT = 0;
constexpr mnemonic_t    MNEMONIC_ADD = 1;
constexpr mnemonic_t    MNEMONIC_SUB = 2;
constexpr mnemonic_t    MNEMONIC_STA = 3;
constexpr mnemonic_t    MNEMONIC_LDA = 4;
constexpr mnemonic_t    MNEMONIC_BRA = 5;
constexpr mnemonic_t    MNEMONIC_BRZ = 6;
constexpr mnemonic_t    MNEMONIC_BRP = 7;
constexpr mnemonic_t    MNEMONIC_INP = 8;
constexpr mnemonic_t    MNEMONIC_OUT = 9;
constexpr mnemonic_t    MNEMONIC_DAT = 10;


std::string g_progname;
int g_pc = 0;
short g_program[100] = {};


bool is_comment(char c)
{
    return c == '#' || c == '/' || c == ';';
}


std::vector<std::string> split(std::string const & input)
{
    std::vector<std::string> words;
    char const * s(input.c_str());
    for(;;)
    {
        while(*s != '\0' && isspace(*s))
        {
            ++s;
        }
        if(*s == '\0' || is_comment(*s))
        {
            return words;
        }
        char const * w(s);
        while(*s != '\0' && !isspace(*s) && !is_comment(*s))
        {
            ++s;
        }
        words.push_back(std::string(w, s - w));
    }
}


mnemonic_t is_mnemonic(std::string const & word)
{
    std::string w;
    for(auto const & c : word)
    {
        if(c >= 'a' && c <= 'z')
        {
            w += c - 0x20;
        }
        else
        {
            w += c;
        }
    }
    if(w == "HLT")
    {
        return MNEMONIC_HLT;
    }
    if(w == "ADD")
    {
        return MNEMONIC_ADD;
    }
    if(w == "SUB")
    {
        return MNEMONIC_SUB;
    }
    if(w == "STA")
    {
        return MNEMONIC_STA;
    }
    if(w == "LDA")
    {
        return MNEMONIC_LDA;
    }
    if(w == "BRA")
    {
        return MNEMONIC_BRA;
    }
    if(w == "BRZ")
    {
        return MNEMONIC_BRZ;
    }
    if(w == "BRP")
    {
        return MNEMONIC_BRP;
    }
    if(w == "INP")
    {
        return MNEMONIC_INP;
    }
    if(w == "OUT")
    {
        return MNEMONIC_OUT;
    }
    if(w == "DAT")
    {
        return MNEMONIC_DAT;
    }

    return MNEMONIC_NONE;
}


bool parse(std::string const & filename)
{
    std::ifstream in;
    in.open(filename);
    if(!in.is_open())
    {
        std::cerr << "error: could not open \"" << filename
            << "\" for reading.\n";
        return false;
    }

    int errcount(0);
    int line(0);
    std::map<std::string, int> label_pc;
    std::map<int, std::string> label_ref;
    std::string input;
    while(std::getline(in, input))
    {
        ++line;
        std::vector<std::string> words(split(input));
        if(words.empty())
        {
            continue;
        }

        std::string parameter;
        mnemonic_t instruction(is_mnemonic(words[0]));
        if(instruction != MNEMONIC_NONE)
        {
            if(words.size() > 2)
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": more than two words on the line is not legal.\n";
                continue;
            }

            // no label
            //
            if(words.size() == 2)
            {
                parameter = words[1];
            }
        }
        else if(words.size() < 2)
        {
            ++errcount;
            std::cerr << "error:" << filename << ":" << line
                << ": a word by itself, which is not a mnemonic, is not legal.\n";
            continue;
        }
        else
        {
            instruction = is_mnemonic(words[1]);
            if(instruction == MNEMONIC_NONE)
            {
                ++errcount;
                std::cerr << "error: " << filename << ":" << line
                    << ": a label must be followed by a mnemonic.\n";
                continue;
            }

            // we have a label -- save its position
            //
            label_pc[words[0]] = g_pc;

            if(words.size() == 3)
            {
                parameter = words[2];
            }
            else if(words.size() > 3)
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": a mnemonic can be followed by at most one parameter.\n";
                continue;
            }
        }

        if(g_pc >= std::size(g_program))
        {
            std::cerr << "error:" << filename << ":" << line
                << ": program too long; limit is 1000 instructions/data.\n";
            continue;
        }

        switch(instruction)
        {
        case MNEMONIC_NONE:
            throw std::logic_error("instruction still undefined.");

        case MNEMONIC_HLT:
            if(!parameter.empty())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": the HTL instruction does not accept a parameter.\n";
            }
            else
            {
                g_program[g_pc] = 0;
                ++g_pc;
            }
            break;

        case MNEMONIC_ADD:
            if(parameter.empty())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": the ADD instruction requires a parameter (label reference).\n";
            }
            else
            {
                g_program[g_pc] = 100;
                label_ref[g_pc] = parameter;
                ++g_pc;
            }
            break;

        case MNEMONIC_SUB:
            if(parameter.empty())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": the SUB instruction requires a parameter (label reference).\n";
            }
            else
            {
                g_program[g_pc] = 200;
                label_ref[g_pc] = parameter;
                ++g_pc;
            }
            break;

        case MNEMONIC_STA:
            if(parameter.empty())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": the STA instruction requires a parameter (label reference).\n";
            }
            else
            {
                g_program[g_pc] = 300;
                label_ref[g_pc] = parameter;
                ++g_pc;
            }
            break;

        case MNEMONIC_LDA:
            if(parameter.empty())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": the LDA instruction requires a parameter (label reference).\n";
            }
            else
            {
                g_program[g_pc] = 400;
                label_ref[g_pc] = parameter;
                ++g_pc;
            }
            break;

        case MNEMONIC_BRA:
            if(parameter.empty())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": the BRA instruction requires a parameter (label reference).\n";
            }
            else
            {
                g_program[g_pc] = 500;
                label_ref[g_pc] = parameter;
                ++g_pc;
            }
            break;

        case MNEMONIC_BRZ:
            if(parameter.empty())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": the BRZ instruction requires a parameter (label reference).\n";
            }
            else
            {
                g_program[g_pc] = 600;
                label_ref[g_pc] = parameter;
                ++g_pc;
            }
            break;

        case MNEMONIC_BRP:
            if(parameter.empty())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": the BRP instruction requires a parameter (label reference).\n";
            }
            else
            {
                g_program[g_pc] = 700;
                label_ref[g_pc] = parameter;
                ++g_pc;
            }
            break;

        case MNEMONIC_INP:
            if(!parameter.empty())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": the INP instruction does not accept a parameter.\n";
            }
            else
            {
                g_program[g_pc] = 800;
                ++g_pc;
            }
            break;

        case MNEMONIC_OUT:
            if(!parameter.empty())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": the OUT instruction does not accept a parameter.\n";
            }
            else
            {
                g_program[g_pc] = 900;
                ++g_pc;
            }
            break;

        case MNEMONIC_DAT:
            if(parameter.empty())
            {
                g_program[g_pc] = 0;
                ++g_pc;
            }
            else
            {
                int const value(atoi(parameter.c_str()));
                if(value > 999)
                {
                    std::cerr << "error:" << filename << ":" << line
                        << ": DAT supports numbers between 0 and 999.\n";
                }
                else
                {
                    g_program[g_pc] = value;
                    ++g_pc;
                }
            }
            break;

        }
    }

    // second pass to enter the label positions
    //
    for(auto const & l : label_ref)
    {
        int number(0);
        for(auto const & c : l.second)
        {
            if(c >= '0' && c <= '9')
            {
                number = number * 10 + c - '0';
            }
            else
            {
                number = -1;
                break;
            }
        }
        if(number != -1)
        {
            if(number > 999)
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": label \"" << l.second
                    << "\" is too large a number.\n";
                continue;
            }
            g_program[l.first] += number;
        }
        else
        {
            auto const f(label_pc.find(l.second));
            if(f == label_pc.end())
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": label \"" << l.second
                    << "\" was not found.\n";
                continue;
            }
            if(f->second > 99)
            {
                ++errcount;
                std::cerr << "error:" << filename << ":" << line
                    << ": offset of label \"" << l.second
                    << "\" is too large (" << f->second
                    << ").\n";
            }
            g_program[l.first] += f->second;
        }
    }

    if(errcount != 0)
    {
        std::cerr << "found " << errcount << " errors.\n";
        return false;
    }

    return true;
}


void execute()
{
    g_pc = 0;
    int acc(0);
    bool overflow(false);

    for(;;)
    {
        int const instruction(g_program[g_pc] / 100);
        int const loc(g_program[g_pc] % 100);
        ++g_pc;
        switch(instruction)
        {
        case MNEMONIC_HLT:
            // done
            return;

        case MNEMONIC_ADD:
            overflow = acc + g_program[loc] > 999;
            acc = (acc + g_program[loc]) % 1000;
            break;

        case MNEMONIC_SUB:
            overflow = acc < g_program[loc];
            acc = (acc - g_program[loc]) % 1000;
            break;

        case MNEMONIC_STA:
            g_program[loc] = acc;
            break;

        case MNEMONIC_LDA:
            acc = g_program[loc];
            break;

        case MNEMONIC_BRA:
            g_pc = loc;
            break;

        case MNEMONIC_BRZ:
            if(acc == 0)
            {
                g_pc = loc;
            }
            break;

        case MNEMONIC_BRP:
            if(!overflow)
            {
                g_pc = loc;
            }
            break;

        case MNEMONIC_INP:
            std::cout << "lmc> " << std::flush;
            {
                int value;
                std::cin >> value;
                acc = value % 1000;
            }
            break;

        case MNEMONIC_OUT:
            std::cout << acc << "\n";
            break;

        }
    }
}


void usage()
{
    std::cout << "Usage: " << g_progname << " [-opts] <file.lmc>\n"
        << "where -opts is one or more of:\n"
        << "   -h          print out this help screen\n";
}

int main(int argc, char * argv[])
{
    g_progname = argv[0];
    std::string::size_type const pos(g_progname.rfind('/'));
    if(pos != std::string::npos)
    {
        g_progname = g_progname.substr(pos + 1);
    }
    bool show(false);
    std::string filename;
    for(int i(1); i < argc; ++i)
    {
        if(argv[i][0] == '-')
        {
            size_t const max(strlen(argv[i]));
            for(size_t j(1); j < max; ++j)
            {
                switch(argv[i][j])
                {
                case 'h':
                    usage();
                    return 1;

                case 's':
                    show = true;
                    break;

                default:
                    std::cerr << "error: unknown command line option '"
                        << argv[i][j]
                        << "'. Try -h for help.\n";
                    return 1;

                }
            }
        }
        else if(filename.empty())
        {
            filename = argv[i];
        }
        else
        {
            std::cerr << "error: enter no more than one filename.\n";
            return 1;
        }
    }

    if(filename.empty())
    {
        std::cerr << "error: filename missing.\n";
        return 1;
    }

    if(!parse(filename))
    {
        return 1;
    }

    if(show)
    {
        for(int pc(0); pc < g_pc; ++pc)
        {
            std::cout << std::setw(3) << pc << ":    " << g_program[pc] << "\n";
        }
        return 0;
    }

    execute();

    return 0;
}

// vim: ts=4 sw=4 et
