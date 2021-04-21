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

size_t ini::refresh() {
  FILE *file = fopen(m_filename.c_str(), "r");

  if (!file)
    return 1;

  // Clear out our variables
  m_sections.clear();
  m_keys.clear();

  // Default section
  m_sections.push_back("default");
  int index = m_sections.size() - 1;

  // Loop through every line
  char *cline = nullptr;
  size_t len = 0;
  while (__getline(&cline, &len, file) != -1) {
    std::string line(cline);
    // Ensure there is no carriage return or line feed
    int lineEnd = line.size() - 1;
    while(line[lineEnd] == '\r' || line[lineEnd] == '\n')
      lineEnd--;
    line = line.substr(0, lineEnd + 1);

    // Ignore if comment
    if (line[0] == ';')
      continue;

    // Check if section
    if (line[0] == '[' && line.find(']') != std::string::npos) {
      // Append if not a duplicate
      bool is_duplicate = false;
      for (size_t i = 0; i < m_sections.size(); i++)
        if (m_sections[i] == line.substr(1, line.find(']') - 1)) {
          is_duplicate = true;
          index = i;
        }
      // Otherwise add
      if (!is_duplicate) {
        m_sections.push_back(line.substr(1, line.find(']') - 1));
        index = m_sections.size() - 1;
      }
    }

    // Check if valid key, then append if it is
    if (line.find('=') != std::string::npos) {
      m_keys.push_back(
          (m_ini_key_t){index, line.substr(0, line.find('=')),
                        line.substr(line.find('=') + 1, line.size())});

      // Remove leading and trailing whitespace
      int start = 0, end = m_keys.back().name.size() - 1;
      while(m_keys.back().name[start] == ' ')
        start++;
      while(m_keys.back().name[end] == ' ')
        end--;
      m_keys.back().name = m_keys.back().name.substr(start, end + 1 - start);

      start = 0, end = m_keys.back().data.size() - 1;
      while(m_keys.back().data[start] == ' ')
        start++;
      while(m_keys.back().data[end] == ' ')
        end--;
      m_keys.back().data = m_keys.back().data.substr(start, end + 1 - start);
    }
  }

  // free(cline);
  fclose(file);
  return 0;
}