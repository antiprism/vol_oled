/*
   Copyright (c) 2018, Adrian Rossiter

   Antiprism - http://www.antiprism.com

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

      The above copyright notice and this permission notice shall be included
      in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.
*/

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#include <math.h>
#include <vector>
#include <string>

using std::vector;
using std::string;


struct ProgOpts
{
  string prog_name;
  string version;
  int oled;
  int framerate;
  int bars;
  int gap;
  unsigned char i2c_addr;

  ProgOpts(): prog_name("vol_oled"), version("0.01"),
              oled(OLED_ADAFRUIT_SPI_128x32), framerate(30), bars(16), gap(1),
              i2c_addr(0)
    {}
  void usage();
  void parse_args(int argc, char *argv[]);
};


void ProgOpts::usage()
{
  printf("%s\n", prog_name.c_str());
  printf("Usage: %s -o oled_type [options]\n", prog_name.c_str());
  printf("  -o <type>  OLED type\nOLED type are:\n");
  for (int i=0; i<OLED_LAST_OLED;i++)
    if (strstr(oled_type_str[i], "128x64"))
      printf("  %1d %s\n", i, oled_type_str[i]);

  printf("Options:\n");
  printf("  -h         help\n");
  printf("  -b <num>   number of bars to display (default: 16)\n");
  printf("  -g <sz>    gap between bars in, pixels (default: 1)\n");
  printf("  -f <hz>    framerate in Hz (default: 30)\n");
  printf("  -a <addr>  I2C address, in hex (default: default for OLED type)\n");
  printf("Example :\n");
  printf( "%s -o 6 use a %s OLED\n\n", prog_name.c_str(), oled_type_str[6]);
}


void ProgOpts::parse_args(int argc, char *argv[])
{
  opterr = 1;  // suppress error message for unrecognised option
  int c;
  while ((c=getopt(argc, argv, ":ho:b:g:f:a:")) != -1)
  {
    switch (c) 
    {
      case 'o':
        oled = (int) atoi(optarg);
        if (oled < 0 || oled >= OLED_LAST_OLED ||
            !strstr(oled_type_str[oled], "128x64")) {
          fprintf(stderr, "error: -o %d: invalid 128x64 oled type (see -h)\n",
              oled);
          exit(EXIT_FAILURE);
        }
        break;

      case 'b':
        bars = (int) atoi(optarg);
        if (oled < 2 || oled > 60) {
          fprintf(stderr, "error: -b %d: select between 2 and 60 bars\n", bars);
          exit(EXIT_FAILURE);
        }
        break;

      case 'g':
        gap = (int) atoi(optarg);
        if (gap < 0 || gap > 30) {
          fprintf(stderr,
              "error: -g %d: select a gap between 0 and 30 pixels\n", gap);
          exit(EXIT_FAILURE);
        }
        break;

      case 'f':
        framerate = (int) atoi(optarg);
        if (framerate < 1) {
          fprintf(stderr,
              "error: -f %d: framerate must be a positive integer\n", framerate);
          exit(EXIT_FAILURE);
        }
        break;

      case 'a':
        if (strlen(optarg) != 2 ||
            strspn(optarg, "01234567890aAbBcCdDeEfF") != 2 ) {
          fprintf(stderr,
              "error: -a %s: I2C address should be two hexadecimal digits\n",
              optarg);
          exit(EXIT_FAILURE);
        }

        i2c_addr = (unsigned char) strtol(optarg, NULL, 16);
        break;

      case 'h':
        usage();
        exit(EXIT_SUCCESS);
        break;

      
      case '?': 
        fprintf(stderr, "error: unrecognized option -%c.\n", optopt);
        fprintf(stderr, "run with '-h'.\n");
        exit(EXIT_FAILURE);
              
      case ':':
        fprintf(stderr, "error: -%c missing argument.\n", optopt);
        fprintf(stderr, "run with '-h'.\n");
        exit(EXIT_FAILURE);
              
      default:
        fprintf(stderr, "error: unknown error\n");
        fprintf(stderr, "run with '-h'.\n");
        exit(EXIT_FAILURE);
    }
  }

  if (oled == 0) {
    fprintf(stderr, "error: must specify a 128x64 oled with -o\n");
    exit(EXIT_FAILURE);
  }
  
  if (bars + (bars-1)*gap > 128) {
    fprintf(stderr, "error: to display %d bars with a gap of %d means that the "
                    "bar width will be less than one. Reduce the number of "
                    "bars and/or the gap.\n", bars, gap);
    exit(EXIT_FAILURE);
  }
}

string print_config_file(int bars, int framerate, string fifo_name)
{
  char templt[] = "/tmp/cava_config_XXXXXX";
  int fd = mkstemp(templt);
  if(fd == -1)
    return "";  // failed to open file and confert to file stream
  FILE *ofile = fdopen(fd, "w");
  if(ofile == NULL)
    return "";  // failed to open file and confert to file stream

  fprintf(ofile, "[general]\n"
                 "framerate = %d\n"
                 "bars = %d\n"
                 "\n"
                 "[input]\n"
                 "method = fifo\n"
                 "source = /tmp/mpd_vol_oled\n"
                 "\n"
                 "[output]\n"
                 "method = raw\n"
                 "data_format = binary\n"
                 "channels = mono\n"
                 "raw_target = %s\n"
                 "bit_format = 8bit\n",
          framerate, bars, fifo_name.c_str());
  fclose(ofile);
  return templt;
}


int draw_spectrum(ArduiPi_OLED &display, int x_start, int y_start, int width,
    int height, const unsigned char vals[], int num_bars, int gap)
{
  int total_bar_pixes = width-(num_bars-1)*gap;
  int bar_width = floor(total_bar_pixes / num_bars);
  int bar_height_max = height - 1;
  int graph_width = num_bars*bar_width + (num_bars-1)*gap;

  if(bar_width < 1 || bar_height_max < 1)  // bars too small to draw
    return -1;

  // Draw spectrum graph axes
  display.drawFastHLine(x_start, height - 1 - y_start, graph_width, WHITE);
  for (int i=0; i<num_bars; i++) {
    int val = bar_height_max * vals[i] / 255.0;  // map vals range to graph ht
    int x = x_start + i*(bar_width+gap);
    int y = y_start+2;
    display.fillRect(x, y_start, bar_width, height - val - 1, BLACK);
    display.fillRect(x, y_start + height - val - 2, bar_width, val, WHITE);
  }
  return 0;
}


void draw_clock(ArduiPi_OLED &display)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  time_t t = time(0);
  struct tm *now = localtime(&t);
  const size_t STR_SZ = 32;
  char str[STR_SZ];
  strftime(str, STR_SZ, "%H:%M", now);
  
  display.setCursor(4,4);
  display.setTextSize(4);
  display.print(str);
  
  
  strftime(str, STR_SZ, "%d-%m-%Y", now);
  display.setCursor(0,46);
  display.setTextSize(2);
  //display.setCursor(30,54);
  //display.setTextSize(1);
  display.print(str);  
}

bool init_display(ArduiPi_OLED &display, int oled, unsigned char i2c_addr)
{
// SPI
  if (display.oled_is_spi_proto(oled)) {
    // SPI change parameters to fit to your LCD
    if ( !display.init(OLED_SPI_DC, OLED_SPI_RESET, OLED_SPI_CS, oled) )
      return false;
  }
  else {
    // I2C change parameters to fit to your LCD
    if ( !display.init(OLED_I2C_RESET, oled, i2c_addr) )
      return false;
  }

  display.begin();

  // init done
  display.clearDisplay();   // clears the screen  buffer
  display.display();   		// display it (clear display)

  return true;
}

int read_and_display_loop(ArduiPi_OLED &display, FILE *fifo_file, int type,
    int bars, int gap)
{
  int cnt = 0;
  int check_every = 100;
  int quiet = 0;
  unsigned char bar_heights[bars];

  int fifo_fd = fileno(fifo_file);

  // Use this variable to clear the display when changing display type
  int cur_disp_type = 0; // 0:none, 1:spectrum, 2:clock
  
  while (true) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fifo_fd, &set);

    // FIFO read timeout value
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // If there is spectrum data, read and display it, otherwise, display clock
    if(select(FD_SETSIZE, &set, NULL, NULL, &timeout)) {
      if(cur_disp_type != 1) {  // not spectrum
        cur_disp_type = 1;
        display.clearDisplay();
      }
      int num_read = fread(bar_heights, sizeof(unsigned char), bars, fifo_file);
      draw_spectrum(display, 0, 0, 128, 64, bar_heights, bars, gap);
    }
    else {
      if(cur_disp_type != 2) {  // not clock
        cur_disp_type = 2;
        display.clearDisplay();
      }
      draw_clock(display);
    }

    display.display();
  }
}


int main(int argc, char **argv)
{
  ProgOpts opts;
  opts.parse_args(argc, argv);

  // Set up the OLED doisplay
  ArduiPi_OLED display;
  if(!init_display(display, opts.oled, opts.i2c_addr)) {
    fprintf(stderr, "error: could not initialise OLED\n");
    exit(EXIT_FAILURE);
  }

  // Create a FIFO for cava to write its raw output to
  const string fifo_name = "/tmp/cava_fifo";
  unlink(fifo_name.c_str());
  if(mkfifo(fifo_name.c_str(), 0666) == -1) {
    perror("could not create cava FIFO for writing");
    exit(EXIT_FAILURE);
  }

  // Create a temporary config file for cava
  string config_file_name = print_config_file(opts.bars, opts.framerate,
      fifo_name);
  if (config_file_name == "") {
    perror("could not create cava config file");
    exit(EXIT_FAILURE);
  }

  // Create a pipe to a cava subprocess
  string cava_cmd = "cava -p " + config_file_name;
  FILE *from_cava = popen(cava_cmd.c_str(), "r");
  if (from_cava == NULL) {
    perror("could not start cava program");
    exit(EXIT_FAILURE);
  }

  // Create a file stream to read cava's raw output from
  FILE *fifo_file = fopen(fifo_name.c_str(), "rb");
  if(fifo_file == NULL) {
    fprintf(stderr, "could not open cava FIFO for reading");
    exit(EXIT_FAILURE);
  }

  int type = 2;
  read_and_display_loop(display, fifo_file, type, opts.bars, opts.gap);
  
  // Free PI GPIO ports
  display.close();

}
