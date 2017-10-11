/***************************************************************************************************
 * simple_image_machine.c
 * ----------------------
 *
 * OCTOBER 10, 2017
 * VERSION 0.5_alt
 *
 * DIRECTIONS:
 * ===========
 * Write a C program that: stamps images:
 * 
 * -- 1. Create 3 or more template structs that are created in their own separate file;
 *       these templates can be manually programmed, or you can load them from a file;
 * -- 2. Create an array of r,g,b values that is 1024x768;
 * -- 3. Using the templates you created in the other files, copy the templates into your
 *       large array;
 * -- 4. Write the array to a file according to the .PPM file specification;
 * -- 5. then convert it to .png using GIMP or another tool.
 *
 * USAGE: 
 * ======
 * 
 * simple_image_machine -o outputfile.ppm duck 40,100 circle 90,500 turkey 600,600 square 200,200
 *
 ***************************************************************************************************/

#include "./simple_image_machine.h"

#define VERSION 0.5_alt
#define USAGE "USAGE: simple_image_machine -o <outputfile>.ppm template1 <x1> <y1> template2 <x2> <y2> ...\n"

/* *************************************************************************
 * MAIN
 * *************************************************************************/
int main(int argc, char** argv) {

  /* FIRST, option processing: -o outputfile | -h */
  char* outputfile = outputFilename(argc, argv);
  fprintf(stderr, "-o %s\n", outputfile);

  /* SECOND, create and initialize with all white an array that holds r,g,b color values */
  PIXEL** imagebuffer = malloc(sizeof(void*) * HEIGHT);
  for (int row = 0; row < HEIGHT; row++) {
    imagebuffer[row] = calloc(WIDTH, PIXEL_S);
  }
  fillBuffer(imagebuffer, WHITE);
  fprintf(stderr, "Created image buffer\n");
  /* displayImageBuffer(imagebuffer); */

  /* THIRD, loop over template files given on command line, load them, and overlay them */
  int f = 3;
  while (f < argc) {
    TEMPLATE* template = loadTemplate(argv, f);
    f += 3;
    
    overlay(imagebuffer, template);
    fprintf(stderr, "Overlaid template data onto image buffer\n");

    /* cleanup template memory */
    free(template->buff);
    free(template);
  }

  /* FOURTH, write out a PPM file to the file system */
  writePPM(outputfile, imagebuffer);
  fprintf(stderr, "DONE\n");

  /* CLEAN UP */
  free(imagebuffer);

  return 0;
}
/***************************************************************************
 * END MAIN
 ***************************************************************************/

/***************************************************************************
 * outputFilename                                                          *
 * --------------                                                          *
 * Uses getopt to process options -o outputfile | -h                       *
 *                                                                         *
 * parameters: int (argc)                                                  *
 *             char* (argv)                                                *
 * returns:    char* (output file name)                                    *
 ***************************************************************************/
char* outputFilename(int argc, char* argv[]) {
  int ch;
  while ((ch = getopt(argc, argv, "ho:")) != -1) { /* -o outputfile.ppm
                                                      -h help */
    if (ch == 'o')
      break;
    if (ch == 'h') {
      fprintf(stderr, USAGE);
      fprintf(stderr, "The simple_image_machine stamps template images onto a main image and produces a PPM (type P6) image file.\n  \
The templates are placed in the image at the points given after the template names.\n  The templates can be larger than or \
extend outside the boundaries of the image, but they will be truncated.\n  The template sizes are given in ascii at the \
beginning of the template file as two numbers and a newline: `xxx yyy \\n'.\n");

      exit(EXIT_SUCCESS);
    }
  }
  if (ch != 'o') {
    fprintf(stderr, "getting option name\n");
    fprintf(stderr, USAGE);
    exit(1);
  }
  return optarg;
}

/***************************************************************************
 * templateInfo                                                            *
 * ------------                                                            *
 * Prints template information for debugging                               *
 *                                                                         *
 * parameter; TEMPLATE*                                                    *
 * returns:   void                                                         *
 ***************************************************************************/
void templateInfo(TEMPLATE* t) {
  fprintf(stderr, "template name: %s size: (%d x %d) stamp point: (%d, %d)\n",
          t->name, t->width, t->height, t->start_x, t->start_y);
}

/***************************************************************************
 * fillBuffer                                                              *
 * ----------                                                              *
 * Initializes a buffer with a fill value                                  *
 *                                                                         *
 * parameters: PIXEL** buffer                                              *
 *             PIXEL fill                                                  *
 * returns:    void                                                        *
 ***************************************************************************/
void fillBuffer(PIXEL** buff, PIXEL fill) {
  for (int row = 0; row < HEIGHT; row++) {
    for (int col = 0; col < WIDTH; col++) {
      buff[row][col] = fill;
    }
  }
}

/***************************************************************************
 * loadTemplate                                                            *
 * ------------                                                            *
 * Loads a template as given on the command line                           *
 *                                                                         *
 * parameters: char** argv (holds template name and stampe point)          *
 *             int (current index into argv)                               *
 * returns:    PIXEL_T (size of template in pixels)                        *
 ***************************************************************************/
TEMPLATE* loadTemplate(char** argv, int f) {
  TEMPLATE* template = malloc(sizeof(TEMPLATE));; /* SOURCE OF ERROR PRODUCING RANDOM SEG FAULTS;
                                                     NEEDED TO MALLOC MEMORY FOR THE TEMPLATE STRUCTURE */

  char* name = argv[f++];
  int start_x = atoi(argv[f++]);
  int start_y = atoi(argv[f++]);

  if ((fp = fopen(name, READ)) != NULL) {
    int width = 0;
    int height = 0;
    
    if ((fscanf(fp, "%d %d\n", &width, &height)) != 2) {
      fprintf(stderr, "ERROR reading width height in template file\n");
      exit(EXIT_FAILURE);
    }
    int templateSize = width * height;

    template->name = name;
    template->start_x = start_x;
    template->start_y = start_y;
    template->width = width;
    template->height = height;
    template->buff = calloc(templateSize, PIXEL_S);
    
    PIXEL_T pixelsRead = fread(template->buff, PIXEL_S, templateSize, fp);
    templateInfo(template);
    printf("pixels read = %d\n", pixelsRead);
    fclose(fp);

  } else {
    fprintf(stderr, "can't open file %s\n", name);
    exit(1);
  }
  return template;
}

/***************************************************************************
 * overlay                                                                 *
 * -------                                                                 *
 * Overlays a template onto the image buffer                               *
 *                                                                         *
 * parameters; TEMPLATE* template                                          *
 * returns:    void                                                        *
 ***************************************************************************/
void overlay(PIXEL** imagebuffer, TEMPLATE* template) {
  int width = template->width;
  int height = template->height;
  int start_x = template->start_x;
  int start_y = template->start_y;
  int p = 0;
  for (int row = start_y; row < start_y + height; row++) {
    for (int col = start_x; col < start_x + width; col++) {
      PIXEL* pixel = template->buff + p;
      if (!(pixel->red == 0 && pixel->green == 0 && pixel->blue == 0)) { 
          imagebuffer[row][col] = *pixel;
        }
      p++;
    }
  }
}

/***************************************************************************
 * writePPM                                                                *
 * --------                                                                *
 * Writes out the image buffer as a PPM (type P6) image file               *
 *                                                                         *
 * parameters: char* outputfile                                            *
 * returns:    void                                                        *
 ***************************************************************************/
void writePPM(char* outputfile, PIXEL** imagebuffer) {
  if ((fp = fopen(outputfile, WRITE)) != NULL) {
    /* http://netpbm.sourceforge.net/doc/ppm.html */
    /* Each PPM image consists of the following:
     *   1. the magic number "P6"; */
    /*   2. Whitespace (blanks, TABs, CRs, LFs). */
    /*   3. A width, formatted as ASCII characters in decimal. */
    /*   4. Whitespace. */
    /*   5. A height, again in ASCII decimal. */
    /*   6. Whitespace. */
    /*   7. The maximum color value (Maxval), again in ASCII decimal. Must be less than 65536 and more than zero. */
    if ((fprintf(fp, "%s\n#%s\n%d %d\n%d\n", PPM_TYPE, outputfile, WIDTH, HEIGHT, MAXVAL)) == EOF) {
      fprintf(stderr, "error writing header information to output file %s\n", outputfile);
      exit(1);
    }

    PIXEL_T written, pixels;
    for (int row = 0; row < HEIGHT; row++) {
      if ((pixels = fwrite(imagebuffer[row], PIXEL_S, WIDTH, fp)) != WIDTH) {
        fprintf(stderr, "ERROR writing image buffer row %d; wrote [%d] bytes out of [%u] possible\n", row, written, BUFSIZE);
        exit(EXIT_FAILURE);
      }
      written += pixels;
    }

    if (written != BUFSIZE) {
      fprintf(stderr, "ERROR writing image buffer; wrote %d pixels out of %u possible\n", written, BUFSIZE);
      exit (EXIT_FAILURE);
    }
    fclose(fp);
    fprintf(stderr, "SUCCESSFULLY wrote %d pixels out of a possible %u possible\n", written, BUFSIZE);
  }

  else {
    fprintf(stderr, "error opening a file for writing to\n");
    exit(EXIT_FAILURE);
  }
}

/***************************************************************************
 * makeColor                                                               *
 * ---------                                                               *
 * Given three colors, make and return a PIXEL                             *
 *                                                                         *
 * parameters; color (red), color (green), color (blue)                    *
 * returns:    PIXEL                                                       *
 ***************************************************************************/
PIXEL makeColor(color red, color green, color blue) {
  return (PIXEL){red, green, blue};
}

/***************************************************************************
 * showColors                                                              *
 * ----------                                                              *
 * Given a PIXEL, print out its components for debugging                   *
 *                                                                         *
 * parameters; PIXEL* p                                                    *
 * returns:    void                                                        *
 ***************************************************************************/
void showColors(PIXEL* p) {
  printf("%x:%x:%x ", p->red & 0xFF, p->green & 0xFF, p->blue & 0xFF);
}

/***************************************************************************
 * displayBuffer                                                           *
 * -------------                                                           *
 * Given a template, print out its components for debugging                *
 *                                                                         *
 * parameters: TEMPLATE* template                                          *
 * returns:    void                                                        *
 ***************************************************************************/
void displayBuffer(TEMPLATE* template) {
  templateInfo(template);
    for (int row = 0; row < template->height; row++) {
      printf("row %2d ", row);
      for (int col = 0; col < template->width; col++) {
        printf("[%d]", col);
        PIXEL* p = template->buff + (row * PIXEL_S * template->width) + (col * PIXEL_S);
        showColors(p);
      }
      putchar('\n');
    }
}

/***************************************************************************
 * displayImageBuffer                                                      *
 * ------------------                                                      *
 * Displays all of the pixels in the image buffer for debugging            *
 *                                                                         *
 * parameters: PIXEL** imagebuffer                                         *
 * returns:    void                                                        *
 ***************************************************************************/
void displayImageBuffer(PIXEL** imagebuffer) {
  for (int row = 0; row < HEIGHT; row++) {
    printf("row [%2d]\n", row);
    for (int col = 0; col < WIDTH; col++) {
      PIXEL p = imagebuffer[row][col];
      printf("[%2d]", col);
      showColors(&p);
    }
    putchar('\n');
  }
}
