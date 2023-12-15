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

#include "wasm_export.h"

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


static char global_heap_buf[4 * 1024];
static NativeSymbol native_symbols[] = {
};

unsigned char __aligned(4) wasm_test_file_interp[] = {
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x10, 0x03, 0x60,
    0x01, 0x7F, 0x01, 0x7F, 0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F, 0x60, 0x01,
    0x7F, 0x00, 0x02, 0x31, 0x04, 0x03, 0x65, 0x6E, 0x76, 0x04, 0x70, 0x75,
    0x74, 0x73, 0x00, 0x00, 0x03, 0x65, 0x6E, 0x76, 0x06, 0x6D, 0x61, 0x6C,
    0x6C, 0x6F, 0x63, 0x00, 0x00, 0x03, 0x65, 0x6E, 0x76, 0x06, 0x70, 0x72,
    0x69, 0x6E, 0x74, 0x66, 0x00, 0x01, 0x03, 0x65, 0x6E, 0x76, 0x04, 0x66,
    0x72, 0x65, 0x65, 0x00, 0x02, 0x03, 0x02, 0x01, 0x01, 0x04, 0x05, 0x01,
    0x70, 0x01, 0x01, 0x01, 0x05, 0x03, 0x01, 0x00, 0x01, 0x06, 0x13, 0x03,
    0x7F, 0x01, 0x41, 0xC0, 0x28, 0x0B, 0x7F, 0x00, 0x41, 0xBA, 0x08, 0x0B,
    0x7F, 0x00, 0x41, 0xC0, 0x28, 0x0B, 0x07, 0x2C, 0x04, 0x06, 0x6D, 0x65,
    0x6D, 0x6F, 0x72, 0x79, 0x02, 0x00, 0x0A, 0x5F, 0x5F, 0x64, 0x61, 0x74,
    0x61, 0x5F, 0x65, 0x6E, 0x64, 0x03, 0x01, 0x0B, 0x5F, 0x5F, 0x68, 0x65,
    0x61, 0x70, 0x5F, 0x62, 0x61, 0x73, 0x65, 0x03, 0x02, 0x04, 0x6D, 0x61,
    0x69, 0x6E, 0x00, 0x04, 0x0A, 0xB2, 0x01, 0x01, 0xAF, 0x01, 0x01, 0x03,
    0x7F, 0x23, 0x80, 0x80, 0x80, 0x80, 0x00, 0x41, 0x20, 0x6B, 0x22, 0x02,
    0x24, 0x80, 0x80, 0x80, 0x80, 0x00, 0x41, 0x9B, 0x88, 0x80, 0x80, 0x00,
    0x10, 0x80, 0x80, 0x80, 0x80, 0x00, 0x1A, 0x02, 0x40, 0x02, 0x40, 0x41,
    0x80, 0x08, 0x10, 0x81, 0x80, 0x80, 0x80, 0x00, 0x22, 0x03, 0x0D, 0x00,
    0x41, 0xA8, 0x88, 0x80, 0x80, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x00,
    0x1A, 0x41, 0x7F, 0x21, 0x04, 0x0C, 0x01, 0x0B, 0x20, 0x02, 0x20, 0x03,
    0x36, 0x02, 0x10, 0x41, 0x80, 0x88, 0x80, 0x80, 0x00, 0x20, 0x02, 0x41,
    0x10, 0x6A, 0x10, 0x82, 0x80, 0x80, 0x80, 0x00, 0x1A, 0x41, 0x00, 0x21,
    0x04, 0x20, 0x03, 0x41, 0x04, 0x6A, 0x41, 0x00, 0x2F, 0x00, 0x91, 0x88,
    0x80, 0x80, 0x00, 0x3B, 0x00, 0x00, 0x20, 0x03, 0x41, 0x00, 0x28, 0x00,
    0x8D, 0x88, 0x80, 0x80, 0x00, 0x36, 0x00, 0x00, 0x20, 0x02, 0x20, 0x03,
    0x36, 0x02, 0x00, 0x41, 0x93, 0x88, 0x80, 0x80, 0x00, 0x20, 0x02, 0x10,
    0x82, 0x80, 0x80, 0x80, 0x00, 0x1A, 0x20, 0x03, 0x10, 0x83, 0x80, 0x80,
    0x80, 0x00, 0x0B, 0x20, 0x02, 0x41, 0x20, 0x6A, 0x24, 0x80, 0x80, 0x80,
    0x80, 0x00, 0x20, 0x04, 0x0B, 0x0B, 0x41, 0x01, 0x00, 0x41, 0x80, 0x08,
    0x0B, 0x3A, 0x62, 0x75, 0x66, 0x20, 0x70, 0x74, 0x72, 0x3A, 0x20, 0x25,
    0x70, 0x0A, 0x00, 0x31, 0x32, 0x33, 0x34, 0x0A, 0x00, 0x62, 0x75, 0x66,
    0x3A, 0x20, 0x25, 0x73, 0x00, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77,
    0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00, 0x6D, 0x61, 0x6C, 0x6C, 0x6F, 0x63,
    0x20, 0x62, 0x75, 0x66, 0x20, 0x66, 0x61, 0x69, 0x6C, 0x65, 0x64, 0x00
};

void wasmTaskInit() {
  wasmInputQueue = STATIC_MEM_QUEUE_CREATE(wasmInputQueue);

  STATIC_MEM_TASK_CREATE(wasmTask, wasmTask, WASM_TASK_NAME, NULL, WASM_TASK_PRI);

  RuntimeInitArgs init_args;
  init_args.mem_alloc_type = Alloc_With_Pool;
  init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
  init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
  // Native symbols need below registration phase
  init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
  init_args.native_module_name = "env";
  init_args.native_symbols = native_symbols;
  if (!wasm_runtime_full_init(&init_args)) {
    isInit = false;
  } else {
    isInit = true;
  }
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

static void *
app_instance_main(wasm_module_inst_t module_inst)
{
    const char *exception;

    wasm_application_execute_main(module_inst, 0, NULL);
    if ((exception = wasm_runtime_get_exception(module_inst)))
        printf("%s\n", exception);
    return NULL;
}

void wasmRunImage(void) {
  // wasm_runtime_full_init -> wasm_runtime_load -> wasm_runtime_instantiate -> wasm_application_execute_main
    uint8_t *wasm_file_buf = NULL;
    unsigned wasm_file_buf_size = 0;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    char error_buf[128];
    void* ret;

  /* load WASM module */
  if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_buf_size,
                                        error_buf, sizeof(error_buf)))) {
    DEBUG_PRINT("Error in wasm_runtime_load: %s", error_buf);
    goto fail1interp;
  }

  DEBUG_PRINT("Instantiate WASM runtime");
  if (!(wasm_module_inst =
          wasm_runtime_instantiate(wasm_module, 32 * 1024, // stack size
                                  32 * 1024,              // heap size
                                  error_buf, sizeof(error_buf)))) {
    DEBUG_PRINT("Error while instantiating: %s", error_buf);
    goto fail2interp;
  }

  DEBUG_PRINT("run main() of the application");
  ret = app_instance_main(wasm_module_inst);

  /* destroy the module instance */
  DEBUG_PRINT("Deinstantiate WASM runtime");
  wasm_runtime_deinstantiate(wasm_module_inst);

fail2interp:
  /* unload the module */
  DEBUG_PRINT("Unload WASM module");
  wasm_runtime_unload(wasm_module);
fail1interp:
  DEBUG_PRINT("Finish");
}

void wasmCtrl(CRTPPacket* p) {
  switch (p->data[0]) {
    case WASM_CTRL_RESET_FILE:
      writeLocation = 0;
      break;
    case WASM_CTRL_RUN:
      wasmRunImage();
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
