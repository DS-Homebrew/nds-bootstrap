// libeasysave

/*
MIT License

Copyright (c) 2019 Jonathan Archer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "easysave/ini.hpp"

using namespace easysave;

/// Writes INI data to the disk. Returns non-zero on fail.
size_t ini::flush() {
  FILE *file = fopen(m_filename.c_str(), "w");
  // Loop through every section and dump its keys to a file
  // TODO: Faster implementation, this is like O(n^2)
  for (size_t i = 0; i < m_sections.size(); i++) {
    // We set the section to empty because we haven't looped for keys yet
    bool section_is_empty = true;
    for (size_t j = 0; j < m_keys.size(); j++)
      if ((size_t)m_keys[j].index == i) {
        // If the section is marked empty, but we are here, then we add the
        // section and mark it non-empty
        if (section_is_empty) {
          section_is_empty = false;
          fwrite("[", 1, 1, file);
          fwrite(m_sections[i].c_str(), 1, m_sections[i].size(), file);
          fwrite("]\n", 1, 2, file);
        }
        // We do a fetch here to avoid duping quotes. Inefficient, and should be
        // redone! Maybe break off the stripping of quotes into a function that
        // also takes a key index?
        fwrite(m_keys[j].name.c_str(), 1, m_keys[j].name.size(), file);
        fwrite("=\"", 1, 2, file);
        std::string temp = fetch(m_sections[m_keys[j].index], m_keys[j].name);
        fwrite(temp.c_str(), 1, temp.size(), file);
        fwrite("\"\n", 1, 2, file);
      }
  }
  fclose(file);
  return 0;
}