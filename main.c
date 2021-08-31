#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

# define INITIAL_BUFFER_SIZE 16

struct GameItem {
	int32_t price;
	char * name;
	float weight;
};

struct Buffer {
	void *data;
	size_t next;
	size_t size;
};

void printGameItem(struct GameItem gameItem);
struct Buffer * makeBuffer();
void freeBuffer(struct Buffer * buffer);
void reserveBufferSpace(struct Buffer * buffer, size_t bytes);
void serializeInt32(int x, struct Buffer * buffer);
void serializeString(char * s, struct Buffer * b);
void serializeFloat(float f, struct Buffer * b);
void serializeGameItem(struct GameItem gameItem, struct Buffer * buffer);
void unserializeGameItem(struct GameItem * gameItem, struct Buffer * buffer);
char * unserializeString(struct Buffer * b, size_t * next);
int32_t unserializeInt32(struct Buffer * b, size_t * next);
float unserializeFloat(struct Buffer * b, size_t * next);
void freeGameItem(struct GameItem * item);

int main()
{
	printf("serializing game item:\n\n");
	struct GameItem blueberries;
	blueberries.name = "blueberries";
	blueberries.price = 15;
	blueberries.weight = 0.5f;

	printGameItem(blueberries);

	struct Buffer * buffer = makeBuffer();
	serializeGameItem(blueberries, buffer);

	printf("\nunserializing game item...\n");

	struct GameItem * unserializedBlueberries = malloc(sizeof(struct GameItem));
	unserializeGameItem(unserializedBlueberries, buffer);

	printGameItem(*unserializedBlueberries);

	freeBuffer(buffer);
	// TODO: write to file

	freeGameItem(unserializedBlueberries);
	return 0;
}

void printGameItem(struct GameItem gameItem)
{
	printf("%s\n", gameItem.name);
	printf("price: %d\n", gameItem.price);
	printf("weight: %f\n", gameItem.weight);
}

struct Buffer * makeBuffer()
{
	struct Buffer * buffer = malloc(sizeof(struct Buffer));
	buffer->data = malloc(INITIAL_BUFFER_SIZE);
	buffer->size = INITIAL_BUFFER_SIZE;
	buffer->next = 0;
	return buffer;
}

void freeBuffer(struct Buffer * buffer)
{
	free(buffer->data);
	free(buffer);
	return;
}

void reserveBufferSpace(struct Buffer * buffer, size_t bytes)
{
	while ((buffer->next + bytes) > buffer->size)
	{
		buffer->data = realloc(buffer->data, buffer->size * 2);
		buffer->size *= 2;
	}
}

void serializeInt32(int32_t x, struct Buffer * buffer)
{
	x = htonl(x);
	reserveBufferSpace(buffer, sizeof(int32_t));
	memcpy(((char *) buffer->data) + buffer->next, &x, sizeof(int32_t));
	buffer->next += sizeof(int32_t);
}

void serializeGameItem(struct GameItem gameItem, struct Buffer * buffer)
{
	serializeString(gameItem.name,  buffer);
	serializeInt32(gameItem.price, buffer);
	serializeFloat(gameItem.weight, buffer);
}

void serializeString(char * s, struct Buffer * b)
{
	size_t sLen = 0;
	{
		char * currentChar = s;
		while (*currentChar != '\0') sLen++, currentChar = s + sLen;
	}
	reserveBufferSpace(b, sLen);
	memcpy(((char *) b->data) + b->next, s, sLen);
	b->next += sLen;
}

// Floats are serialized as text hexadecimal exponent notation.
void serializeFloat(float f, struct Buffer * serialBuffer)
{
	size_t fBufferSize = snprintf(NULL, 0, "%a", f);
	char * fBuffer = malloc(fBufferSize);
	sprintf(fBuffer, "%a", f);
	reserveBufferSpace(serialBuffer, fBufferSize);
	memcpy(((char *) serialBuffer->data) + serialBuffer->next, fBuffer, fBufferSize);
	serialBuffer->next += fBufferSize;
}

void unserializeGameItem(struct GameItem * gameItem, struct Buffer * buffer)
{	
	size_t next = 0;
	gameItem->name = unserializeString(buffer, &next);
	gameItem->price = unserializeInt32(buffer, &next);
	gameItem->weight = unserializeFloat(buffer, &next);
}

char * unserializeString(struct Buffer * b, size_t * next)
{
	size_t sLen = snprintf(NULL, 0, ((char *) b->data) + *next, "%s");
	char * s = malloc(sLen);
	sprintf(s, "%s", ((char *) b->data) + *next);
	*next = *next + sLen;
	return s;
}

// Floats are serialized as text hexadecimal exponent notation.
int32_t unserializeInt32(struct Buffer * b, size_t * next)
{
	int32_t x;
 	memcpy(&x, ((char *) b->data) + *next, sizeof(int32_t));
	x = ntohl(x);
	*next += sizeof(int32_t);
	return x;
}

float unserializeFloat(struct Buffer * b, size_t * next)
{
	size_t sLen = snprintf(NULL, 0, ((char *) b->data) + *next, "%s");
	char * fBuffer = malloc(sLen);
	sprintf(fBuffer, "%s", ((char *) b->data) + *next);
	float f = strtod(fBuffer, NULL);
	*next = *next + sLen;
	return f;
}

void freeGameItem(struct GameItem * item)
{
	free(item->name);
	free(item);
}
