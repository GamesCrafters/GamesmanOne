/**
 * @file opening_credits.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Per-frame animation of the opening credits in interactive mode.
 * @version 1.0.0
 * @date 2025-04-26
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef NDEBUG
#define THEME "\x1b[38;5;166m"
#else
#define THEME
#endif
#define RESET "\x1b[0m"

// clang-format off
static const char *const kHeaderAnimation[] = {
    THEME"                    "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    "                                                        "RESET": complete  information"THEME"\n"
    "                                                        "RESET": game generator.  More"THEME"\n"
    "                                                        "RESET": information?  Contact"THEME"\n"
    "                                                        "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"                    "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    "                                                        "RESET": complete  information"THEME"\n"
    "                                                        "RESET": game generator.  More"THEME"\n"
    "                                                        "RESET": information?  Contact"THEME"\n"
    "                                                        "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"                    "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    "      |                                                 "RESET": complete  information"THEME"\n"
    "                                                        "RESET": game generator.  More"THEME"\n"
    "                                                        "RESET": information?  Contact"THEME"\n"
    "                                                        "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"     _              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    "     _|                                                 "RESET": complete  information"THEME"\n"
    "                                                        "RESET": game generator.  More"THEME"\n"
    "                                                        "RESET": information?  Contact"THEME"\n"
    "                                                        "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"    __              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    "    __|                                                 "RESET": complete  information"THEME"\n"
    "                                                        "RESET": game generator.  More"THEME"\n"
    "                                                        "RESET": information?  Contact"THEME"\n"
    "                                                        "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"   ___              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    "   ___|                                                 "RESET": complete  information"THEME"\n"
    "                                                        "RESET": game generator.  More"THEME"\n"
    "                                                        "RESET": information?  Contact"THEME"\n"
    "                                                        "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___|                                                 "RESET": complete  information"THEME"\n"
    "                                                        "RESET": game generator.  More"THEME"\n"
    "                                                        "RESET": information?  Contact"THEME"\n"
    "                                                        "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___|                                                 "RESET": complete  information"THEME"\n"
    "| |                                                     "RESET": game generator.  More"THEME"\n"
    "                                                        "RESET": information?  Contact"THEME"\n"
    "                                                        "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___|                                                 "RESET": complete  information"THEME"\n"
    "| |                                                     "RESET": game generator.  More"THEME"\n"
    "| |                                                     "RESET": information?  Contact"THEME"\n"
    "                                                        "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___|                                                 "RESET": complete  information"THEME"\n"
    "| |                                                     "RESET": game generator.  More"THEME"\n"
    "| |                                                     "RESET": information?  Contact"THEME"\n"
    " \\__                                                    "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___|                                                 "RESET": complete  information"THEME"\n"
    "| |                                                     "RESET": game generator.  More"THEME"\n"
    "| |_                                                    "RESET": information?  Contact"THEME"\n"
    " \\___                                                   "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___|                                                 "RESET": complete  information"THEME"\n"
    "| |                                                     "RESET": game generator.  More"THEME"\n"
    "| |_                                                    "RESET": information?  Contact"THEME"\n"
    " \\____|                                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___|                                                 "RESET": complete  information"THEME"\n"
    "| |                                                     "RESET": game generator.  More"THEME"\n"
    "| |_| |                                                 "RESET": information?  Contact"THEME"\n"
    " \\____|                                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___|                                                 "RESET": complete  information"THEME"\n"
    "| |  _                                                  "RESET": game generator.  More"THEME"\n"
    "| |_| |                                                 "RESET": information?  Contact"THEME"\n"
    " \\____|                                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___|                                                 "RESET": complete  information"THEME"\n"
    "| |  _    `                                             "RESET": game generator.  More"THEME"\n"
    "| |_| |                                                 "RESET": information?  Contact"THEME"\n"
    " \\____|                                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___|  _                                              "RESET": complete  information"THEME"\n"
    "| |  _   _`                                             "RESET": game generator.  More"THEME"\n"
    "| |_| |                                                 "RESET": information?  Contact"THEME"\n"
    " \\____|                                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __                                              "RESET": complete  information"THEME"\n"
    "| |  _   _`                                             "RESET": game generator.  More"THEME"\n"
    "| |_| |                                                 "RESET": information?  Contact"THEME"\n"
    " \\____|                                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __                                              "RESET": complete  information"THEME"\n"
    "| |  _ / _`                                             "RESET": game generator.  More"THEME"\n"
    "| |_| | (                                               "RESET": information?  Contact"THEME"\n"
    " \\____|                                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __                                              "RESET": complete  information"THEME"\n"
    "| |  _ / _`                                             "RESET": game generator.  More"THEME"\n"
    "| |_| | (                                               "RESET": information?  Contact"THEME"\n"
    " \\____|\\                                                "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __                                              "RESET": complete  information"THEME"\n"
    "| |  _ / _`                                             "RESET": game generator.  More"THEME"\n"
    "| |_| | (_                                              "RESET": information?  Contact"THEME"\n"
    " \\____|\\_                                               "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __                                              "RESET": complete  information"THEME"\n"
    "| |  _ / _`                                             "RESET": game generator.  More"THEME"\n"
    "| |_| | (_                                              "RESET": information?  Contact"THEME"\n"
    " \\____|\\__                                              "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __                                              "RESET": complete  information"THEME"\n"
    "| |  _ / _`                                             "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| |                                           "RESET": information?  Contact"THEME"\n"
    " \\____|\\__                                              "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __                                              "RESET": complete  information"THEME"\n"
    "| |  _ / _` |                                           "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| |                                           "RESET": information?  Contact"THEME"\n"
    " \\____|\\__                                              "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _                                            "RESET": complete  information"THEME"\n"
    "| |  _ / _` |                                           "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| |                                           "RESET": information?  Contact"THEME"\n"
    " \\____|\\__                                              "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _                                            "RESET": complete  information"THEME"\n"
    "| |  _ / _` |                                           "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| |                                           "RESET": information?  Contact"THEME"\n"
    " \\____|\\__, |                                           "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _                                            "RESET": complete  information"THEME"\n"
    "| |  _ / _` |                                           "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| |                                           "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|                                           "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _                                          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '                                         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| |                                           "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|                                           "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _                                          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '                                         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | |                                         "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|                                           "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _                                          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '                                         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | |                                         "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_| |                                         "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _                                          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '                                         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | |                                         "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_|                                         "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ _                                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_                                        "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | |                                         "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_|                                         "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __                                       "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ `                                      "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | |                                         "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_|                                         "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __                                       "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ `                                      "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | |                                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_|                                         "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __                                       "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ `                                      "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | |                                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| | |                                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __                                       "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ `                                      "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | |                                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_|                                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ __                                    "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _                                    "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | |                                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_|                                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___                                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\                                  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | |                                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_|                                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___                                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\                                  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |                                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_|                                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___                                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\                                  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |                                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| | |                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___                                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\                                  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |                                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___                                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\                                  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |                                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___                                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\                                  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  _                              "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___                                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\   _                              "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __                             "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___                                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\   _                              "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___                                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\   _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___     _                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\   _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___    __                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\   _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\   _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|                                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\                                "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\_                               "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\__                              "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___                             "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\                            "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___                             "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\    |                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___    _                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\   _|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___   __                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\  __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\  __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/                            "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\                           "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\_                          "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__                         "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\                       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|                            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\                       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|    /                       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\                       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|   _/                       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\                       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___|  __/                       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\                       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___| ___/                       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\                       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/                       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___                        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\                       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/                       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _                      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __|                       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\                       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/                       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _                      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '                     "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\                       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/                       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _                      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '                     "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ |                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/                       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _                      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '                     "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ |                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/ |                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _                      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '                     "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ |                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_|                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ _                    "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_                    "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ |                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_|                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_                    "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ |                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_|                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ `                  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ |                     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_|                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ `                  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | |                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_|                     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ `                  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | |                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| | |                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __                   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ `                  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | |                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_|                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ _                 "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _                "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | |                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_|                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ __                "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _                "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | |                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_|                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___               "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _                "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | |                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_|                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___               "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\              "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | |                 "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_|                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___               "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\              "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | |             "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_|                 "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___               "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\              "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | |             "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| | |             "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___               "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\              "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | |             "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|             "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___               "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\              "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | |             "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|             "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___               "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\    `         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | |             "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|             "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___    _          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\   _`         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | |             "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|             "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\   _`         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | |             "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|             "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _`         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | |             "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|             "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _`         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (           "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|             "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _`         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (           "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\            "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _`         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_          "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\_           "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _`         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_          "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__          "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _`         "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| |       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__          "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __          "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` |       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| |       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__          "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` |       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| |       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__          "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` |       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| |       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__, |       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _        "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` |       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| |       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` |       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| |       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` |       "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| |       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '     "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| |       "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '     "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | |     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|       "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '     "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | |     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_| |     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _      "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '     "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | |     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|_|     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _ _    "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '_    "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | |     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|_|     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _ __   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '_    "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | |     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|_|     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _ __   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '_ \\  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | |     "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|_|     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _ __   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '_ \\  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | | | | "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|_|     "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _ __   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '_ \\  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | | | | "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|_| | | "RESET": ddgarcia@berkeley.edu"THEME"\n",
    THEME"  ____              "RESET"https://gamescrafters.berkeley.edu  "RESET": A finite,  two-person"THEME"\n"
    " / ___| __ _ _ __ ___   ___  ___ _ __ ___   __ _ _ __   "RESET": complete  information"THEME"\n"
    "| |  _ / _` | '_ ` _ \\ / _ \\/ __| '_ ` _ \\ / _` | '_ \\  "RESET": game generator.  More"THEME"\n"
    "| |_| | (_| | | | | | |  __/\\__ \\ | | | | | (_| | | | | "RESET": information?  Contact"THEME"\n"
    " \\____|\\__,_|_| |_| |_|\\___||___/_| |_| |_|\\__,_|_| |_| "RESET": ddgarcia@berkeley.edu"THEME"\n",
};
// clang-format on
