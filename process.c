#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#define BUFSIZE 256
#define DBSIZE 256
#define CONFIG_FILE "config"
#define UNKNOWN_FILE "unknown.pdf"
#define FILENAME 0
#define KEYPHRASE 1
#define PAGENUM 2

void kalaidoscope(char**);
int initDB();
void freeDB();
int get_total_pages(char* input_file);
int match(char* input_file);
int matchPdf(char* input_file, char* output_file);
int matchScannedPdf(char* input_file, char* output_file);
void splitPdf(int start_page, int end_page, char* input_file, char* output_file);
void mergePdf(char* input_dir, char* output_file);
void appendPdf(char* input_file, char* output_file);
void convertPdfToTxt(char* input_file, char* output_file);
void convertScannedPdfToTxt(char* input_file, char* output_file);
char*** db;
int total_forms;

/* takes an input pdf file and organize it into files based on user's config */
int main(int argc, char** argv) {

  if (argc <= 2) {
    printf("\n\tprocess\t<input file>");
    printf("\n\tsplit\t<input file>\t<start page #>\t<end page #>\t<output file>");
    printf("\n\tmerge\t<input dir>\t<output file>");
    printf("\n\n");
    return 0;
  }

  if (!strcmp(argv[1], "process")) {
    kalaidoscope(argv);
  } else if (!strcmp(argv[1], "split")) {
    if (argc != 6) {
        printf("\nsplit input_file start_page end_page output_file");
        return 0;
    }
    splitPdf(atoi(argv[3]), atoi(argv[4]), argv[2], argv[5]);
  } else if (!strcmp(argv[1], "merge")) {
    if (argc != 4) {
        printf("\nmerge input_dir output_file");
        return 0;
    }
    mergePdf(argv[2], argv[3]);
  }
}

void kalaidoscope(char** argv) {
  int start_page = 1;
  int end_page = 1;
  int num_pages;
  char* cmd = NULL;
  char* input_file = NULL;
  FILE* fp;
  int dbindex = 0;
  
  if (initDB() == -1) {
    printf("Error initializing DB\n");
    return;
  }

  input_file = argv[1];
  num_pages = get_total_pages(input_file);
  printf("=== Processing %s (%d pages) ===\n", input_file, num_pages);

  while (end_page <= num_pages) {
    splitPdf(start_page, end_page, input_file, "kalaidoscope.pdf");
    dbindex = matchPdf("kalaidoscope.pdf", "kalaidoscope.txt");
    if (dbindex == -1) {
      dbindex = matchScannedPdf("kalaidoscope.pdf", "kalaidoscope.txt");
    }
    if (dbindex > -1) {
      end_page = start_page + atoi(db[dbindex][PAGENUM]) - 1;
      splitPdf(start_page, end_page, input_file, db[dbindex][FILENAME]);
      end_page++;
      start_page = end_page;
    } else {
      appendPdf("kalaidoscope.pdf", UNKNOWN_FILE);
      start_page++;
      end_page++;
    }
  }

  remove("kalaidoscope.pdf");
  remove("kalaidoscope.txt");
  freeDB();
  free(cmd);
}

int matchPdf(char* input_file, char* output_file) {
  convertPdfToTxt(input_file, output_file);
  return match(output_file);
}

int matchScannedPdf(char* input_file, char* output_file) {
  // check for scanned pdf case:
  // use OCR to convert pdf and check for text
  convertScannedPdfToTxt(input_file, output_file);
  return match(output_file);
}

void freeDB() {
  int i=0;
  while (db[i]) {
    free(db[i][FILENAME]);
    free(db[i][KEYPHRASE]);
    free(db[i][PAGENUM]);
    i++;
  }
}

int initDB() {
  char* buf = malloc(BUFSIZE);
  int i=0;
  db = (char***)malloc(DBSIZE*sizeof(char*));
  FILE* fp;

  remove(UNKNOWN_FILE);
  fp = fopen(UNKNOWN_FILE, "ab+");

  if ((fp = fopen(CONFIG_FILE, "r")) == NULL) {
    printf("Error opening config file (%s)\n", CONFIG_FILE);
    return -1;
  }

  while (fgets(buf, BUFSIZE, fp) != NULL) {
    db[i] = (char**)malloc(3*sizeof(char*));
    db[i][FILENAME] = (char*)malloc(BUFSIZE);
    db[i][KEYPHRASE] = (char*)malloc(BUFSIZE);
    db[i][PAGENUM] = (char*)malloc(BUFSIZE);
    sscanf(buf, "%79[^,],%79[^,],%4[^,]", db[i][FILENAME], db[i][KEYPHRASE], db[i][PAGENUM]);
    i++;
  }
  total_forms = i;

  fclose(fp);
  free(buf);

  return 0;
}

void splitPdf(int start_page, int end_page, char* input_file, char* output_file) {
  char* cmd = (char*)malloc(BUFSIZE);
  sprintf(cmd, "gs -dBATCH -sOutputFile=%s -dFirstPage=%d -dLastPage=%d -sDEVICE=pdfwrite %s",
          output_file, start_page, end_page, input_file);
  system(cmd);
  free(cmd);
}


void mergePdf(char* input_dir, char* output_file) {
  char* cmd = (char*)malloc(BUFSIZE);
  char* input_file = (char*)malloc(BUFSIZE);
  char* temp= (char*)malloc(BUFSIZE);
  DIR *dir = opendir(input_dir);
  struct dirent *dp;

  if (readdir(dir) == NULL) {
    printf("input_dir is not valid\n");
    return;
  }

  while ((dp=readdir(dir)) != NULL) {
    if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") || !strncmp(dp->d_name, ".", 1)) {
        continue;
    }
    sprintf(temp, " %s/%s", input_dir, dp->d_name);
    strcat(input_file, temp);
  }
  sprintf(cmd, "gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -dPDFSETTINGS=/prepress -sOutputFile=%s %s",
	output_file, input_file);
  system(cmd);
  free(cmd);
  free(temp);
  free(input_file);
}

void appendPdf(char* input_file, char* output_file) {
  char* cmd = (char*)malloc(BUFSIZE);
  sprintf(cmd, "cp %s temp.pdf", output_file);
  system(cmd);
  sprintf(cmd, "gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -dPDFSETTINGS=/prepress -sOutputFile=%s temp.pdf %s",
          output_file, input_file);
  system(cmd);
  remove("temp.pdf");
  free(cmd);
}

void convertPdfToTxt(char* input_file, char* output_file) {
  char* cmd = (char*)malloc(BUFSIZE);
  sprintf(cmd, "gs -sDEVICE=txtwrite -o %s %s", output_file, input_file);
  system(cmd);
  free(cmd);
}

void convertScannedPdfToTxt(char* input_file, char* output_file) {
  char* cmd = (char*)malloc(BUFSIZE);
  char* buf = malloc(BUFSIZE);
  FILE *ifp = NULL;
  FILE *ofp = fopen(output_file, "w+");
  sprintf(cmd, "curl --form \"file=@%s\" --form \"apikey=070cb4ee7188957\" --form \"language=eng\" -form \"isOverlayRequired=true\" https://api.ocr.space/Parse/Image", input_file);
  if ((ifp = popen(cmd, "r"))) {
    while (fgets(buf, BUFSIZE, ifp) != NULL) {
      fprintf(ofp, "%s\n", buf);
    }
  }
  pclose(ifp);
  fclose(ofp);
  free(buf);
  free(cmd);
}

int match(char* input_file) {
  char* buf = malloc(BUFSIZE);
  int page_num;
  FILE *fp; 

  if ((fp = fopen(input_file, "r")) == NULL) {
    printf("Error opening %s\n", input_file);
    return -1;
  }

  while(fgets(buf, BUFSIZE, fp) != NULL) {
    for (int i=0; i<total_forms; i++) {
      if (strstr(buf, db[i][1])) {
        free(buf);
        fclose(fp);
        return i;
      }
    }
  }

  fclose(fp);
  free(buf);
  return -1;
}

int get_total_pages(char *input_file) {
  char buf[BUFSIZE];
  int page_start, page_num = -1;
  FILE *fp;
  char* cmd = (char*)malloc(BUFSIZE);

  sprintf(cmd, "gs -sDEVICE=txtwrite -o output.txt %s", input_file);
  remove("output.txt");

  if ((fp = popen(cmd, "r")) == NULL) {
    printf("Error running command %s\n", cmd);
    return -1;
  }

  while (fgets(buf, BUFSIZE, fp) != NULL) {
    if (!strncmp(buf, "Processing pages", 16)) {
        sscanf(buf, "Processing pages %d through %d", &page_start, &page_num);
        break;
    }
  }

  pclose(fp);
  free(cmd);
  return page_num;
}
