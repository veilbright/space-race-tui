#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <syslog.h>

static struct option long_options[] = {
  { "textfile", required_argument, NULL, 'f' },
  { 0, 0, 0, 0 },
};

/*
 * Returns all the text from filename (removes trailing whitespace)
 * The pointer to the text must be freed
 * Returns NULL if there are any errors
 */
char* get_text(const char* filename) {
  char* text = NULL;

  FILE* fs = fopen(filename, "r");
  if (fs == NULL) {
    syslog(LOG_ERR, "failed to open textfile '%s': %d\n", filename, errno);
    return NULL;
  }

  // Find text size
  if (fseek(fs, 0, SEEK_END) == -1) {
    syslog(LOG_ERR, "failed to fseek textfile '%s': %d\n", filename, errno);
    goto fs_error;
  }

  long text_size = ftell(fs);
  if (text_size == -1) {
    syslog(LOG_ERR, "failed to find textfile '%s' offset: %d", filename, errno);
    goto fs_error;
  }

  // Return to start of text
  rewind(fs);

  // Allocate text + null terminator
  text = malloc(text_size + 1);
  if (text == NULL) {
    syslog(LOG_ERR, "failed to malloc char* for textfile '%s': %d", filename, errno);
    goto fs_error;
  }

  // Read text
  size_t read_size = fread(text, 1, text_size, fs);
  int ferrno;
  if ((ferrno = ferror(fs)) != 0) {
    syslog(LOG_ERR, "failed to read text from textfile '%s': %d", filename, ferrno);
    goto text_error;
  }

  // Strip trailing whitespace
  char* end = &text[read_size - 1];
  while (end > text && isspace(*end)) {
    --end;
  }
  *(end + 1) = '\0';

  // Close file stream
  if (fclose(fs) != 0) {
    syslog(LOG_ERR, "failed to close textfile '%s': %d", filename, errno);
    return NULL;
  }
  return text;

  // F
text_error:
  free(text);

fs_error:
  if (fclose(fs) != 0) {
    syslog(LOG_ERR, "failed to close textfile '%s': %d", filename, errno);
    return NULL;
  }
  return NULL;
}

int main(int argc, char* argv[]) {
  const char* short_options = "f:";
  int c_opt;
  char* textfile;
  char* text;

  openlog("spacerace", LOG_CONS | LOG_PID, LOG_USER);

  while ((c_opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
    switch (c_opt) {
      case 'f':
        textfile = strdup(optarg);
        if (textfile == NULL) {
          syslog(LOG_ERR, "failed to copy textfile name '%s': %d\n", optarg, errno);
          exit(EXIT_FAILURE);
        }
        break;
    }
  }

  // TODO: printf error checking
  text = get_text(textfile);
  if (text != NULL) {
    printf("%s\n", get_text(textfile));
  }
  else {
    printf("failed to open textfile: %s\n", textfile);
  }

  free(textfile);
  free(text);

  return EXIT_SUCCESS;
}
