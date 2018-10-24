
#define CMSPAR         010000000000

static int ima_index_table[16] = {
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8
}; 

#define MAX_STEP 89
#define PREDICT_LIMIT 32767
#define SHIFT(x) (x>>2)
static int ima_step_table[MAX_STEP] = { 
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 
  19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 
  50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 
  130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
  337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
  876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 
  2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
  5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767 
};

typedef struct {
  int predictor;
  int step_index;
  int step;
} IMA;

typedef struct {
  void (*init)(int fd);
  void (*call)(int fd, char *number);
  void (*send)(int fd, unsigned char *code);
  void (*hangup)(int fd);
  int (*read_tone)(int fd, int mezzisecondi, void *data);
  int (*free_busy)(int fd, void *data);
} Modem;

