#include "config.h"
#include "debug.h"

/* FreeRtos includes */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "static_mem.h"
#include "crtp.h"
#include "system.h"
#include <stdio.h>


char wasmFile[2048];
static uint32_t writeLocation = 0;

static xQueueHandle wasmInputQueue;
STATIC_MEM_QUEUE_ALLOC(wasmInputQueue, 1, sizeof(int));

typedef enum {
  WASM_WRITE_CH,
  WASM_CTRL_CH
} PacketChannel_t;

typedef enum {
  WASM_CTRL_RESET_FILE,
  WASM_CTRL_RUN
} WasmControlType_t;

static void wasmTask(void*);
static void wasmWriteData(CRTPPacket* p);
static const char* channelToString(PacketChannel_t channel);
static void wasmPrintPacket(CRTPPacket* p);
static void wasmCtrl(CRTPPacket* p);

STATIC_MEM_TASK_ALLOC(wasmTask, WASM_TASK_STACKSIZE);

static bool isInit = false;

static CRTPPacket packet;



void wasmTaskInit() {
  wasmInputQueue = STATIC_MEM_QUEUE_CREATE(wasmInputQueue);

  STATIC_MEM_TASK_CREATE(wasmTask, wasmTask, WASM_TASK_NAME, NULL, WASM_TASK_PRI);
  isInit = true;
}

bool wasmTaskTest() {
  return isInit;
}

const char* channelToString(PacketChannel_t channel) {
  switch(channel) {
    case WASM_WRITE_CH:
      return "WASM_WRITE_CH";
    case WASM_CTRL_CH:
      return "WASM_CTRL_CH";
  }

  return "UNKNOWN";
}

void wasmPrintPacket(CRTPPacket* p) {
  DEBUG_PRINT("WASM Packet: ");
  DEBUG_PRINT("  Header: %x", p->header);
  DEBUG_PRINT("  Channel: %s", channelToString(p->channel));
  DEBUG_PRINT("  Size: %d", p->size);
  char data_string[255];
  for (int i = 0; i < p->size; i++) {
    sprintf(data_string, "%x, ", p->data[i]);
  }
  DEBUG_PRINT("  Data: %s", data_string);
}

void wasmWriteData(CRTPPacket* p) {
  for (uint32_t i = 0; i < p->size; i++) {
    wasmFile[writeLocation++] = p->data[i];
  }

  p->header = CRTP_HEADER(CRTP_PORT_WASM, WASM_WRITE_CH);
  p->size = 1;
  p->data[0] = p->data[0]; // send back number; TBD: CRC
  crtpSendPacketBlock(p);
}

void wasmCtrl(CRTPPacket* p) {
  switch (p->data[0]) {
    case WASM_CTRL_RESET_FILE:
      writeLocation = 0;
      break;
    case WASM_CTRL_RUN:
      DEBUG_PRINT("Start executing WASM File");
      // TODO
      break;
    default:
      DEBUG_PRINT("Unknown WASM Control Type");
  }
}

void wasmTaskEnqueueInput(int value) {
  xQueueOverwrite(wasmInputQueue, &value);
}

static void wasmTask(void* parameters) {
  crtpInitTaskQueue(CRTP_PORT_WASM);

  systemWaitStart();

  DEBUG_PRINT("Example task main function is running!");
  while (true) {
    crtpReceivePacketBlock(CRTP_PORT_WASM, &packet);
    DEBUG_PRINT("Received WASM packet with packet number %d", packet.data[0]);
    wasmPrintPacket(&packet);

    switch (packet.channel) {
      case WASM_WRITE_CH:
        wasmWriteData(&packet);
        break;
      case WASM_CTRL_CH:
        wasmCtrl(&packet);
        break;
      default:
        DEBUG_PRINT("Received unknown packet");
    }

    /*
    int input;
    if (pdTRUE == xQueueReceive(wasmInputQueue, &input, portMAX_DELAY)) {
      // Respond to input here!
    }
    */
  }
}