Doxygen Conventions for Gamesman

1. General rules
    a. All source files should have a file header. Refer to section 2 for the
       rules on how to format file headers.
    b. All identifiers (functions, types, variables, enums, macros, etc.) in
       C/C++ header files (.h, .hpp) must be documented using rules in sections
       3-5. There are no requirements or rules on how C/C++ source files (.c,
       .cc, .cpp) may be documented. However, comments are still highly
       encouraged for complex logics and code segments that may be hard to read
       or understand.
    c. Use Javadoc-style comment blocks that start with "/**"
    d. Use the @ symbol for all tags
    e. All identifiers must be highlighted as inline code using Markdown syntax
       by enclosing them in backticks. Always prefer Markdown syntax to inline
       tagging (such as @c).
    f. Project-wide 80-character line width limit also applies to comment blocks
       unless doing so is impossible or significantly reduces readability.
    g. There should be exactly one empty line above each out-of-line comment
       block, including one-liners and those inside structs and enums, with the
       only exception of the file headers, where no empty lines should be left.
2. File rules
    a. All files headers should use the same format that includes the following
       tags in the exact order: @file, @author, @brief, @copyright. Do not 
       include any other tags.
    b. When there are multiple authors, list each author under its own @author
       tag.
    c. Use the exact text as the copyright notice for all source files.
    d. If the file is dual licensed because, for example, it was adapted from
       another open source project, place the original authors and copyright 
       notice above ours. Importantly, do not modify the original copyright
       notice or break it into sections.
3. Function and function-like macro rules
    a. Only out-of-line comment blocks are allowed for documenting functions.
    b. Functions may use the following tags in the exact order: @brief,
       @deprecated, @details, @warning, @note, @tparam (C++), @param, @pre,
       @invariant, @post, @returns, @retval, @throws (C++), @since, @bug, @see,
       @sa.
    c. All @param tags must strictly follow the left-to-right order of the 
       arguments in the actual C/C++ function signature.
    d. All @param tags should use parameter direction annotations (@param\[in\], 
       @param\[out\], @param\[in,out\]) to explicitly state the direction of data
       flow. Note that there is no space after the comma in @param\[in,out\].
    e. Use @return to describe the type of data being passed back and @retval 
       directly underneath it to document specific, discrete values that carry
       special meaning. The @retval tags may be used only when all possible
       return values are enumerated, in which case the @returns tag may be 
       omitted.
    f. Markdown formatting of HTTP links (\[Text\](URL)) are highly encouraged
       to reduce cluttering in generated document. If a link is longer than 2
       lines, consider placing it under the @see or @sa tags.
    g. The links provided in the @see and @sa sections should NOT use the
       Markdown formatting.
    h. The same tags should be grouped by inserting empty lines between each
       group of tags. There are two exceptions to this rule: @returns and 
       @retval must be grouped together and @see and @sa must be grouped
       together.
    i. In the special case where a single function was copied from an open
       source project, place the original license on top of the function
       documentation. If the original source does not come with a license (e.g.,
       copied from Stack Overflow), list the author and source under a 
       @copyright tag.
4. Extern variable and constant macro rules
    a. Use inline comment syntax that starts with "/**<" only for comments that
       fit in one line (80 characters) and do not use any tags. Use a single
       out-of-line comment block on top of the definition otherwise.
5. Struct and enum rules
    a. Use a single out-of-line comment block on top of the definition for the
       struct or enum itself.
    b. Do not use any tags when documenting a member field unless it is a
       function pointer, in which case the same rules for functions apply.
    c. Use in-line comment syntax that starts with "/**<" only if all comments
       fit on the same line (80 characters) with the corresponding members.\
       If not, use a single out-of-line comment block without any tags for
       all members.
6. Typedef rules
    a. For typedefs that serve as simple type aliases (including structs and
       enums), place a single out-of-line comment block above the definition. 
    b. For function pointer typedefs, treat the definition as a function and
       apply the function rules.


Appendix. Gamesman Copyright Notice
Embed the copyright notice below in the header of each source file.

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
