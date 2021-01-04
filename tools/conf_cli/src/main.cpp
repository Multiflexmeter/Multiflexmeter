#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdint.h>
#include <stdio.h>

using namespace std;

struct __attribute__((packed)) rom_conf_t
{
  uint8_t MAGIC[4];
  uint16_t VERSION;
  uint8_t APP_EUI[8];
  uint8_t DEV_EUI[8];
  uint8_t APP_KEY[16];
};

void write_eep_file(rom_conf_t *c, string path);
void upload_eep(string path);

int main(void)
{
  rom_conf_t conf = {
      .MAGIC = {'M', 'F', 'M', 0x00},
      .VERSION = 0x0001,
      .APP_EUI = {0xEC, 0x5B, 0x02, 0xD0, 0x7E, 0xD5, 0xB3, 0x70},
      .DEV_EUI = {0x76, 0x9E, 0x94, 0xDD, 0xAA, 0xA7, 0x02, 0x00},
      .APP_KEY = {0xA1, 0x75, 0xBF, 0xB8, 0x84, 0x3D, 0x78, 0x43, 0xA1, 0x2C, 0xCF, 0x80, 0xD6, 0x28, 0xF7, 0x4B},
  };
  string path = "./eeprom.bin";

  write_eep_file(&conf, path);
  upload_eep(path);

  return 0;
}

void write_eep_file(rom_conf_t *c, string path)
{
  ofstream file;
  file.open(path);
  file.write((const char *)c, sizeof(rom_conf_t));
  file.close();
}

void upload_eep(string path)
{
  stringstream ss;
  ss << "avrdude -c stk500 -p m328p -P /dev/ttyACM0 -U eeprom:w:" << path << ":r";
  system(ss.str().c_str());
}