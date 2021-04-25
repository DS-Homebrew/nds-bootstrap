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

#pragma once

#include <string>
#include <vector>

/// This namespace is the 'root' of everything in the library.
namespace easysave {

/// An INI file object.
class ini {
public:
  // Constructors
  /// Specify the filename and load file into memory.
  ini(std::string filename) {
    update_filename(filename);
    refresh();
  }
  /// Uses 'config.ini' as the filename and loads the file into memory.
  ini() {
    update_filename("config.ini");
    refresh();
  }

  // Filename functions
  /// Returns the filename of the INI object.
  std::string filename() const { return m_filename; }
  /// Changes the filename of the INI object.
  void update_filename(std::string filename) { m_filename = filename; }

  // Methods for fetching keys
  std::string fetch(std::string section, std::string key_name);
  std::string fetch(std::string section, std::string key_name,
                    std::string default_value);

  // Method for setting keys
  void set(std::string section, std::string key_name, std::string key_data);

  // Functions for flushing to disk and refreshing from disk
  size_t flush();
  size_t refresh();

private:
  std::string m_filename;

  // Key structure
  typedef struct {
    int index; // Index in m_sections
    std::string name;
    std::string data;
  } m_ini_key_t;

  // Internal ini data storage variables
  std::vector<std::string> m_sections;
  std::vector<m_ini_key_t> m_keys;

  // Internal helper functions
  inline int m_match_section_index(std::string &section) const {
    // Get index for section
    for (size_t i = 0; i < m_sections.size(); i++)
      if (!section.compare(m_sections[i]))
        return i; // Section found
    return -1;    // Section not found
  }
  inline int m_match_key_index(int section_index, std::string &key_name) const {
    // Loop through keys to find ones with a matching index and name
    for (size_t i = 0; i < m_keys.size(); i++)
      if (m_keys[i].index == section_index && !key_name.compare(m_keys[i].name))
        return i; // Key found
    return -1;    // Key not found
  }
};

} // namespace easysave