#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUFFER_SIZE 16
#define GAME_ITEM_FILE_BUFFER_SIZE 64

struct GameItem {
  int32_t price;
  char *name;
  float weight;
};

struct Buffer {
  void *data;
  size_t next;
  size_t size;
};

void printGameItem(struct GameItem gameItem);
struct GameItem *getGameItemFromFile(char *path);
void saveGameItemToFile(struct GameItem gameItem, char *filePath);
struct Buffer *makeBuffer();
void freeBuffer(struct Buffer *buffer);
void reserveBufferSpace(struct Buffer *buffer, size_t bytes);
void serializeInt32(int x, struct Buffer *buffer);
void serializeString(char *s, struct Buffer *b);
void serializeFloat(float f, struct Buffer *b);
void serializeGameItem(struct GameItem gameItem, struct Buffer *buffer);
void unserializeGameItem(struct GameItem *gameItem, struct Buffer *buffer);
char *unserializeString(struct Buffer *b, size_t *next);
int32_t unserializeInt32(struct Buffer *b, size_t *next);
float unserializeFloat(struct Buffer *b, size_t *next);
void freeGameItem(struct GameItem *item);

int main() {

  // Game Item serialization
  struct GameItem potion;
  potion.name = "potion";
  potion.price = 100;
  potion.weight = 2.0f;
  saveGameItemToFile(potion, "potion");
  printf("Game Item serialized and saved to file \"potion\":\n");
  printGameItem(potion);
  printf("\n");

  // Game Item unserialization
  struct GameItem *unserializedPotion = getGameItemFromFile("potion");
  printf("Game Item unserialized from file \"potion\":\n");
  printGameItem(*unserializedPotion);
  freeGameItem(unserializedPotion);

  return 0;
}

void printGameItem(struct GameItem gameItem) {
  printf("%s\n", gameItem.name);
  printf("price: %d\n", gameItem.price);
  printf("weight: %f\n", gameItem.weight);
}

struct Buffer *makeBuffer() {
  struct Buffer *buffer = malloc(sizeof(struct Buffer));
  buffer->data = malloc(INITIAL_BUFFER_SIZE);
  buffer->size = INITIAL_BUFFER_SIZE;
  buffer->next = 0;
  return buffer;
}

void freeBuffer(struct Buffer *buffer) {
  free(buffer->data);
  free(buffer);
  return;
}

void reserveBufferSpace(struct Buffer *buffer, size_t bytes) {
  while ((buffer->next + bytes) > buffer->size) {
    buffer->data = realloc(buffer->data, buffer->size * 2);
    buffer->size *= 2;
  }
}

void serializeInt32(int32_t x, struct Buffer *buffer) {
  x = htonl(x);
  reserveBufferSpace(buffer, sizeof(int32_t));
  memcpy(((char *)buffer->data) + buffer->next, &x, sizeof(int32_t));
  buffer->next += sizeof(int32_t);
}

void serializeGameItem(struct GameItem gameItem, struct Buffer *buffer) {
  serializeString(gameItem.name, buffer);
  serializeInt32(gameItem.price, buffer);
  serializeFloat(gameItem.weight, buffer);
}

void serializeString(char *s, struct Buffer *b) {
  size_t sLen = 0;
  {
    char *currentChar = s;
    while (*currentChar != '\0')
      sLen++, currentChar = s + sLen;
  }
  reserveBufferSpace(b, sLen);
  memcpy(((char *)b->data) + b->next, s, sLen);
  b->next += sLen;
}

// Floats are serialized as text hexadecimal exponent notation.
void serializeFloat(float f, struct Buffer *serialBuffer) {
  size_t fBufferSize = snprintf(NULL, 0, "%a", f);
  char *fBuffer = malloc(fBufferSize);
  sprintf(fBuffer, "%a", f);
  reserveBufferSpace(serialBuffer, fBufferSize);
  memcpy(((char *)serialBuffer->data) + serialBuffer->next, fBuffer,
         fBufferSize);
  serialBuffer->next += fBufferSize;
}

void unserializeGameItem(struct GameItem *gameItem, struct Buffer *buffer) {
  size_t next = 0;
  gameItem->name = unserializeString(buffer, &next);
  gameItem->price = unserializeInt32(buffer, &next);
  gameItem->weight = unserializeFloat(buffer, &next);
}

char *unserializeString(struct Buffer *b, size_t *next) {
  size_t sLen = snprintf(NULL, 0, ((char *)b->data) + *next, "%s");
  char *s = malloc(sLen);
  sprintf(s, "%s", ((char *)b->data) + *next);
  *next = *next + sLen;
  return s;
}

// Floats are serialized as text hexadecimal exponent notation.
int32_t unserializeInt32(struct Buffer *b, size_t *next) {
  int32_t x;
  memcpy(&x, ((char *)b->data) + *next, sizeof(int32_t));
  x = ntohl(x);
  *next += sizeof(int32_t);
  return x;
}

float unserializeFloat(struct Buffer *b, size_t *next) {
  size_t sLen = snprintf(NULL, 0, ((char *)b->data) + *next, "%s");
  char *fBuffer = malloc(sLen);
  sprintf(fBuffer, "%s", ((char *)b->data) + *next);
  float f = strtod(fBuffer, NULL);
  *next = *next + sLen;
  return f;
}

void freeGameItem(struct GameItem *item) {
  free(item->name);
  free(item);
}

struct GameItem *getGameItemFromFile(char *path) {
  // Make buffer to read file into.
  struct Buffer *fileBuffer = makeBuffer();
  reserveBufferSpace(fileBuffer, GAME_ITEM_FILE_BUFFER_SIZE);

  // Read bytes from file into fileBuffer.
  FILE *file;
  if ((file = fopen(path, "r")) == NULL) {
    printf("could not open file\n");
    char *str_error = strerror(errno);
    printf("error: %s\n", str_error);
    exit(1);
  }
  fread(fileBuffer->data, fileBuffer->size, 1, file);
  fclose(file);

  // Construct gameItem from file buffer.
  struct GameItem *gameItem = malloc(sizeof(struct GameItem));
  unserializeGameItem(gameItem, fileBuffer);
  freeBuffer(fileBuffer);

  return gameItem;
}

void saveGameItemToFile(struct GameItem gameItem, char *filePath) {
  struct Buffer *buffer = makeBuffer();
  serializeGameItem(gameItem, buffer);

  FILE *file;
  if ((file = fopen(filePath, "wb")) == NULL) {
    printf("error opening file\n");
    char *str_error = strerror(errno);
    printf("error: %s\n", str_error);
    exit(1);
  }

  fwrite(buffer->data, buffer->size, 1, file);

  fclose(file);
}
