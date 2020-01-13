/*
 * Kernel
 */

#include "types.h"
#include "kernel.h"
#include "hw86.h"
#include "ulib/ulib.h"
#include "syscall.h"
#include "fs.h"
#include "video.h"
#include "net.h"
#include "cli.h"

uchar a20_enabled = 0; /* A20 line enabled */

uchar serial_status = 0; /* Serial port status */
uint serial_debug = 1; /* Debug info through serial port */

ul_t system_timer_freq = 0; /* Actual frequency of timer */
ul_t system_timer_ms = 10; /* ms since timer initialized */
/* Initialize to 10 ms so all 0 timestamps are outdated */

/* Mouse state */
uint mouse_x = 0; /* Position */
uint mouse_y = 0;
uint mouse_b = 0; /* Buttons */

uint graphics_mode = 1;  /* Graphics mode enabled/disabled (default) */
uint screen_width_c  = 80; /* Screen size (text mode cells) */
uint screen_height_c = 50;
uint screen_width_px  = 800; /* Screen size (graphics mode pixels) */
uint screen_height_px = 600;

/*
 * Disk related
 */
uchar  system_disk; /* System disk */
struct diskinfo disk_info[MAX_DISK];  /* Disk info */

#define KERN_MEMSEG 0x0800L

/*
 * Heap related
 * This memory is only for kernel usage
 */
#define HEAP_MAX_BLOCK   0x0080U
#define HEAP_MEM_SIZE    0x0400U
#define HEAP_BLOCK_SIZE  (HEAP_MEM_SIZE / HEAP_MAX_BLOCK)

static uchar HEAPADDR[HEAP_MEM_SIZE]; /* Allocate heap memory */

struct heapblock_info {
  uint   used;
  void*  ptr;
} heap[HEAP_MAX_BLOCK];

/*
 * Init heap: all blocks are unused
 */
static void heap_init()
{
  uint i = 0;
  for(i=0; i<HEAP_MAX_BLOCK; i++) {
    heap[i].used = 0;
    heap[i].ptr = 0;
  }
}

/*
 * Allocate memory in heap
 */
static void* heap_alloc(uint size)
{
  uint n_alloc = 0;
  uint n_found = 0;
  uint i=0, j=0;

  if(size == 0) {
    return 0;
  }

  /* Get number of blocks to allocate */
  n_alloc = size / HEAP_BLOCK_SIZE +
    ((size % HEAP_BLOCK_SIZE) ? 1 : 0);

  /* Find a continuous set of n_alloc free blocks */
  for(i=0; i<HEAP_MAX_BLOCK; i++) {
    if(heap[i].used) {
      n_found = 0;
    } else {
      n_found++;
      if(n_found >= n_alloc) {
        uint bi = (i+1-n_alloc)*HEAP_BLOCK_SIZE;
        void* addr = &HEAPADDR[bi];
        for(j=i+1-n_alloc; j<=i; j++) {
          heap[j].ptr = addr;
          heap[j].used = 1;
        }

        return addr;
      }
    }
  }

  /* Error: not found */
  debugstr("Mem alloc: BAD ALLOC (%d bytes)\n\r", size);
  return 0;
}

/*
 * Free memory in heap
 */
static heap_free(void* ptr)
{
  uint i = 0;
  if(ptr != 0) {
    for(i=0; i<HEAP_MAX_BLOCK; i++) {
      if(heap[i].ptr == ptr && heap[i].used) {
        heap[i].used = 0;
        heap[i].ptr = 0;
      }
    }
  }

  return;
}

/*
 * Far memory handling
 * Public memory
 */
#define LMEM_START 0x00028000L
#define LMEM_LIMIT 0x0009FC00L
#define LMEM_BLOCK_SIZE 0x10L
#define LMEM_MAX_BLOCK 64
struct lmemblock_info {
  lp_t start;
  ul_t size;
} lmem[LMEM_MAX_BLOCK];

/*
 * Init far memory: all blocks are unused
 */
static void lmem_init()
{
  memset(&lmem, 0, sizeof(lmem));
}

/*
 * Allocate far memory
 */
static lp_t lmem_alloc(ul_t size)
{
  lp_t start = 0;
  uint i = 0;

  /* If size is 0 or all blocks are used, return */
  if(size==0 || lmem[LMEM_MAX_BLOCK-1].size!=0) {
    return 0;
  }

  /* Align blocks to 16 bytes */
  if(size % LMEM_BLOCK_SIZE) {
    size += LMEM_BLOCK_SIZE - size%LMEM_BLOCK_SIZE;
  }

  /* Find a continuous big enough free space */
  for(i=0; i<LMEM_MAX_BLOCK; i++) {
    /* If this block is allocated */
    if(lmem[i].start) {
      /* If there are no more blocks, set not found and break */
      if(i == LMEM_MAX_BLOCK-1) {
        start = 0;
        break;
      }
      /* A possible start is at the end of this block */
      start = lmem[i].start + lmem[i].size;
      /* If there is enough space, break */
      if(lmem[i+1].start==0 || lmem[i+1].start>start+size) {
        i++; /* The right place for the new block is after current block */
        break;
      }
    } else {
      /* Next blocks are free. If it's the first, set start address */
      if(start == 0) {
        start = LMEM_START;
      }
      /* Set not found if there isn't enough space */
      if(LMEM_LIMIT-start < size) {
        start = 0;
      }
      break;
    }
  }

  /* Found if start != 0 */
  if(start != 0) {

    /* Allocate block at i */
    memcpy(&lmem[i+1], &lmem[i],
      (LMEM_MAX_BLOCK-i-1)*sizeof(struct lmemblock_info));

    lmem[i].start = start;
    lmem[i].size = size;

    debugstr("LMem alloc: %X, %U bytes\n\r", lmem[i].start, lmem[i].size);

    return start;
  }

  /* Error: not found */
  debugstr("LMem alloc: BAD ALLOC (%U bytes)\n\r", size);
  return 0;
}

/*
 * Free far memory
 */
static void lmem_free(lp_t ptr)
{
  uint i = 0;
  if(ptr != 0) {
    while(i<LMEM_MAX_BLOCK) {
      /* Find block */
      if(lmem[i].start == ptr) {
        /* Free block */
        lmem[i].start = 0;
        lmem[i].size = 0;

        /* Keep the list sorted and contiguous */
        memcpy(&lmem[i], &lmem[i+1],
          (LMEM_MAX_BLOCK-i-1)*sizeof(struct lmemblock_info));
        lmem[LMEM_MAX_BLOCK-1].start = 0;
        lmem[LMEM_MAX_BLOCK-1].size = 0;
      } else {
        i++;
      }
    }
  }

  return;
}

/*
 * BCD to int
 */
static uint BCD_to_int(uchar BCD)
{
    uint h = (BCD >> 4) * 10;
    return h + (BCD & 0xF);
}

/*
 * Handle system calls
 * Usually:
 * -Unpack parameters
 * -Redirect to the right kernel function
 * -Pack and return parameters
 */
uint kernel_service(uint cs, uint service, lp_t lparam)
{
  switch(service) {

    case SYSCALL_IO_GET_VIDEO_MODE:
      return graphics_mode==0?VM_TEXT:VM_GRAPHICS;

    case SYSCALL_IO_SET_VIDEO_MODE: {
      uint mode = 0;
      lmemcpy(lp(&mode), lparam, lsizeof(mode));
      if(mode==VM_TEXT && graphics_mode==0) {
        io_set_text_mode();
      } else if(mode==VM_GRAPHICS && graphics_mode==1) {
        io_set_graphics_mode();
      }
      return 0;
    }

    case SYSCALL_IO_GET_SCREEN_SIZE: {
      syscall_position_t ps;
      lmemcpy(lp(&ps), lparam, lsizeof(ps));
      if(ps.x == SSM_CHARS) {
        lmemcpy(ps.px, lp(&screen_width_c), lsizeof(screen_width_c));
        lmemcpy(ps.py, lp(&screen_height_c), lsizeof(screen_height_c));
      } else {
        lmemcpy(ps.px, lp(&screen_width_px), lsizeof(screen_width_px));
        lmemcpy(ps.py, lp(&screen_height_px), lsizeof(screen_height_px));
      }
      return 0;
    }

    case SYSCALL_IO_CLEAR_SCREEN:
      io_clear_screen();
      return 0;

    case SYSCALL_IO_SET_PIXEL: {
      syscall_posattr_t ca;
      lmemcpy(lp(&ca), lparam, lsizeof(ca));
      video_set_pixel(ca.x, ca.y, ca.c);
      return 0;
    }

    case SYSCALL_IO_DRAW_CHAR: {
      syscall_posattr_t ca;
      lmemcpy(lp(&ca), lparam, lsizeof(ca));
      draw_char(ca.x, ca.y, ca.c, ca.attr, NO_BACKGROUND);
      return 0;
    }

    case SYSCALL_IO_OUT_CHAR:
      io_out_char(lmem_getbyte(lparam));
      return 0;

    case SYSCALL_IO_OUT_CHAR_ATTR: {
      syscall_posattr_t ca;
      lmemcpy(lp(&ca), lparam, lsizeof(ca));
      io_out_char_attr(ca.x, ca.y, ca.c, ca.attr);
      return 0;
    }

    case SYSCALL_IO_SET_CURSOR_POS: {
      syscall_position_t ps;
      lmemcpy(lp(&ps), lparam, lsizeof(ps));
      io_set_cursor_pos(ps.x, ps.y);
      return 0;
    }

    case SYSCALL_IO_GET_CURSOR_POS: {
      syscall_position_t ps;
      lmemcpy(lp(&ps), lparam, lsizeof(ps));
      io_get_cursor_pos(&ps.x, &ps.y);
      lmemcpy(ps.px, lp(&ps.x), lsizeof(ps.x));
      lmemcpy(ps.py, lp(&ps.y), lsizeof(ps.y));
      return 0;
    }

    case SYSCALL_IO_SET_SHOW_CURSOR: {
      uint mode=0;
      lmemcpy(lp(&mode), lparam, lsizeof(mode));
      if(mode == HIDE_CURSOR) {
        io_hide_cursor();
      } else {
        io_show_cursor();
      }
      return 0;
    }

    case SYSCALL_IO_IN_KEY: {
      uint mode=0, k=0;
      lmemcpy(lp(&mode), lparam, lsizeof(mode));
      do {
        k = io_in_key();
      } while((k==0 && mode==KM_WAIT_KEY) ||
        (k!=0 && mode==KM_CLEAR_BUFFER));

      if(k != 0) {
        if(getHI(k)==(KEY_DEL    >> 8) ||
           getHI(k)==(KEY_END    >> 8) ||
           getHI(k)==(KEY_DEL    >> 8) ||
           getHI(k)==(KEY_HOME   >> 8) ||
           getHI(k)==(KEY_INS    >> 8) ||
           getHI(k)==(KEY_PG_DN  >> 8) ||
           getHI(k)==(KEY_PG_UP  >> 8) ||
           getHI(k)==(KEY_PRT_SC >> 8) ||
           getHI(k)==(KEY_UP     >> 8) ||
           getHI(k)==(KEY_LEFT   >> 8) ||
           getHI(k)==(KEY_RIGHT  >> 8) ||
           getHI(k)==(KEY_DOWN   >> 8) ||
           getHI(k)==(KEY_F1     >> 8) ||
           getHI(k)==(KEY_F2     >> 8) ||
           getHI(k)==(KEY_F3     >> 8) ||
           getHI(k)==(KEY_F4     >> 8) ||
           getHI(k)==(KEY_F5     >> 8) ||
           getHI(k)==(KEY_F6     >> 8) ||
           getHI(k)==(KEY_F7     >> 8) ||
           getHI(k)==(KEY_F8     >> 8) ||
           getHI(k)==(KEY_F9     >> 8) ||
           getHI(k)==(KEY_F10    >> 8) ||
           getHI(k)==(KEY_F11    >> 8) ||
           getHI(k)==(KEY_F12    >> 8)) {
          k &= 0xFF00;
        } else {
          k &= 0x00FF;
        }
      }

      return k;
    }

    case SYSCALL_IO_GET_MOUSE_STATE: {
      syscall_posattr_t pa;
      lmemcpy(lp(&pa), lparam, lsizeof(pa));
      pa.x = mouse_x;
      pa.y = mouse_y;
      pa.c = (mouse_b & 0x3);
      if(pa.attr==SSM_CHARS && graphics_mode) {
        pa.x /= (screen_width_px/screen_width_c);
        pa.y /= (screen_height_px/screen_height_c);
      }
      lmemcpy(lparam, lp(&pa), lsizeof(pa));
      return 0;
    }

    case SYSCALL_IO_OUT_CHAR_SERIAL:
      io_out_char_serial(lmem_getbyte(lparam));
      return 0;

    case SYSCALL_IO_IN_CHAR_SERIAL:
      return (uint)io_in_char_serial();

    case SYSCALL_IO_OUT_CHAR_DEBUG:
      if(serial_debug) {
        io_out_char_serial(lmem_getbyte(lparam));
      }
      return 0;

    case SYSCALL_MEM_ALLOCATE: {
      uint size = 0;

      /* Can't allocate outside kernel segment */
      if(cs != KERN_MEMSEG) {
        return 0;
      }
      lmemcpy(lp(&size), lparam, lsizeof(size));
      return heap_alloc(size);
    }

    case SYSCALL_MEM_FREE:
      heap_free(lparam);
      return 0;

    case SYSCALL_LMEM_ALLOCATE: {
      syscall_lmem_t lm;
      lmemcpy(lp(&lm), lparam, lsizeof(lm));
      lm.dst = lmem_alloc(lm.n);
      lmemcpy(lparam, lp(&lm), lsizeof(lm));
      return 0;
    }
    case SYSCALL_LMEM_FREE: {
      syscall_lmem_t lm;
      lmemcpy(lp(&lm), lparam, lsizeof(lm));
      lmem_free(lm.dst);
      return 0;
    }
    case SYSCALL_LMEM_GET: {
      syscall_lmem_t lm;
      uint i = 0;
      for(i=0; i<lsizeof(lm); i++) {
        ((uint8_t*)(&lm))[i] = lmem_getbyte(lparam+i);
      }
      return lmem_getbyte(lm.dst);
    }
    case SYSCALL_LMEM_SET: {
      syscall_lmem_t lm;
      uint i = 0;
      for(i=0; i<lsizeof(lm); i++) {
        ((uint8_t*)(&lm))[i] = lmem_getbyte(lparam+i);
      }
      lmem_setbyte(lm.dst, (uint8_t)lm.n);
      return 0;
    }

    case SYSCALL_FS_GET_INFO: {
      syscall_fsinfo_t fi;
      fs_info_t info;
      uint result = 0;
      lmemcpy(lp(&fi), lparam, lsizeof(fi));
      result = fs_get_info(fi.disk_index, &info);
      lmemcpy(fi.info, lp(&info), lsizeof(info));
      return result;
    }

    case SYSCALL_FS_GET_ENTRY: {
      syscall_fsentry_t fi;
      sfs_entry_t entry;
      fs_entry_t o_entry;
      uchar path[MAX_PATH];
      uint result = 0;
      lmemcpy(lp(&fi), lparam, lsizeof(fi));
      lmemcpy(lp(path), fi.path, lsizeof(path));
      result = fs_get_entry(&entry, path, fi.parent, fi.disk);
      strcpy_s(o_entry.name, entry.name, sizeof(o_entry.name));
      o_entry.flags = entry.flags;
      o_entry.size = entry.size;
      lmemcpy(fi.entry, lp(&o_entry), lsizeof(o_entry));
      return result;
    }

    case SYSCALL_FS_READ_FILE: {
      syscall_fsrwfile_t fi;
      uchar path[MAX_PATH];
      uint offset = 0;
      lmemcpy(lp(&fi), lparam, lsizeof(fi));
      lmemcpy(lp(path), fi.path, lsizeof(path));
      while(offset < fi.count) {
        uchar tbuff[BLOCK_SIZE];
        uint count = min(sizeof(tbuff), fi.count-offset);
        uint read = fs_read_file(tbuff, path, fi.offset+offset, count);
        if(read >= ERROR_ANY) {
          offset = read;
          break;
        }
        if(read == 0) {
          break;
        }
        lmemcpy(fi.buff+(lp_t)offset, lp(tbuff), (ul_t)read);
        offset += read;
      }
      return offset;
    }

    case SYSCALL_FS_WRITE_FILE: {
      syscall_fsrwfile_t fi;
      uchar path[MAX_PATH];
      uint offset = 0;
      lmemcpy(lp(&fi), lparam, lsizeof(fi));
      lmemcpy(lp(path), fi.path, lsizeof(path));
      while(offset < fi.count) {
        uchar tbuff[BLOCK_SIZE];
        uint write = 0;
        uint count = min(sizeof(tbuff), fi.count-offset);
        lmemcpy(lp(tbuff), fi.buff+(lp_t)offset, (ul_t)count);

        write = fs_write_file(tbuff, path, fi.offset+offset, count, fi.flags);
        if(write >= ERROR_ANY) {
          offset = write;
          break;
        }
        offset += write;
      }
      return offset;
    }

    case SYSCALL_FS_MOVE: {
      syscall_fssrcdst_t fi;
      uchar src[MAX_PATH];
      uchar dst[MAX_PATH];
      lmemcpy(lp(&fi), lparam, lsizeof(fi));
      lmemcpy(lp(src), fi.src, lsizeof(src));
      lmemcpy(lp(dst), fi.dst, lsizeof(dst));
      return fs_move(src, dst);
    }

    case SYSCALL_FS_COPY: {
      syscall_fssrcdst_t fi;
      uchar src[MAX_PATH];
      uchar dst[MAX_PATH];
      lmemcpy(lp(&fi), lparam, lsizeof(fi));
      lmemcpy(lp(src), fi.src, lsizeof(src));
      lmemcpy(lp(dst), fi.dst, lsizeof(dst));
      return fs_copy(src, dst);
    }

    case SYSCALL_FS_DELETE: {
      uchar path[MAX_PATH];
      lmemcpy(lp(path), lparam, lsizeof(path));
      return fs_delete(path);
    }

    case SYSCALL_FS_CREATE_DIRECTORY: {
      uchar path[MAX_PATH];
      lmemcpy(lp(path), lparam, lsizeof(path));
      return fs_create_directory(path);
    }

    case SYSCALL_FS_LIST: {
      syscall_fslist_t fi;
      sfs_entry_t entry;
      fs_entry_t o_entry;
      uchar path[MAX_PATH];
      uint result = 0;
      lmemcpy(lp(&fi), lparam, lsizeof(fi));
      lmemcpy(lp(path), fi.path, lsizeof(path));
      result = fs_list(&entry, path, fi.n);
      strcpy_s(o_entry.name, entry.name, sizeof(o_entry.name));
      o_entry.flags = entry.flags;
      o_entry.size = entry.size;
      lmemcpy(fi.entry, lp(&o_entry), lsizeof(o_entry));
      return result;
    }

    case SYSCALL_FS_FORMAT:
      return fs_format(lmem_getbyte(lparam));

    case SYSCALL_CLK_GET_TIME: {
      time_t t;
      uchar BCDtime[3];
      uchar BCDdate[3];
      get_time(BCDtime, BCDdate);
      t.hour   = BCD_to_int(BCDtime[0]);
      t.minute = BCD_to_int(BCDtime[1]);
      t.second = BCD_to_int(BCDtime[2]);
      t.year   = BCD_to_int(BCDdate[0]) + 2000;
      t.month  = BCD_to_int(BCDdate[1]);
      t.day    = BCD_to_int(BCDdate[2]);
      lmemcpy(lparam, lp(&t), lsizeof(t));
      return 0;
    }

    case SYSCALL_CLK_GET_MILISEC: {
      lmemcpy(lparam, lp(&system_timer_ms), lsizeof(system_timer_ms));
      return 0;
    }

    case SYSCALL_NET_RECV: {
      syscall_netop_t no;
      uint8_t addr[4];
      uint8_t buff[512];
      uint result = 0;
      lmemcpy(lp(&no), lparam, lsizeof(no));
      result = net_recv(addr, buff, no.size);
      lmemcpy(no.addr, lp(addr), lsizeof(addr));
      lmemcpy(no.buff, lp(buff), (ul_t)no.size);
      return result;
    }

    case SYSCALL_NET_SEND: {
      syscall_netop_t no;
      uint8_t addr[4];
      uint8_t buff[512];
      lmemcpy(lp(&no), lparam, lsizeof(no));
      lmemcpy(lp(addr), no.addr, lsizeof(addr));
      lmemcpy(lp(buff), no.buff, (ul_t)no.size);
      return net_send(addr, buff, no.size);
    }
  }

  return 0;
}

/*
 * Mouse IRQ handler
 */
void mouse_handler()
{
  static uchar mouse_cycle = 0;
  static char mouse_byte[3];
  char m_x = 0;
  char m_y = 0;

  switch(mouse_cycle) {
  case 0:
    mouse_byte[0] = inb(0x60);
    mouse_cycle++;
    break;
  case 1:
    mouse_byte[1] = inb(0x60);
    mouse_cycle++;
    break;
  case 2:
    mouse_byte[2] = inb(0x60);
    mouse_cycle = 0;

    /* Update global state data */
    mouse_b = mouse_byte[0];
    m_x = mouse_byte[1];
    m_y = mouse_byte[2];
    mouse_x += m_x >= 0x80 ? m_x-0x100 : m_x;
    mouse_y -= m_y >= 0x80 ? m_y-0x100 : m_y;

    /* Clamp to screen */
    if((int)mouse_x < 0) {
      mouse_x = 0;
    } else if(graphics_mode && mouse_x>screen_width_px) {
      mouse_x = screen_width_px;
    } else if(!graphics_mode && mouse_x>screen_width_c) {
      mouse_x = screen_width_c;
    }
    if((int)mouse_y < 0) {
      mouse_y = 0;
    } else if(graphics_mode && mouse_y>screen_height_px) {
      mouse_y = screen_height_px;
    } else if(!graphics_mode && mouse_y>screen_height_c) {
      mouse_y = screen_height_c;
    }
    break;
  default:
    mouse_cycle = 0;
    break;
  }
}

/*
 * Wait mouse data
 */
#define MS_WAIT_DATA   0
#define MS_WAIT_SIGNAL 1
void mouse_wait(uchar type)
{
  uint time_out = 0x00FF;
  if(type == MS_WAIT_DATA) {
    while(time_out--) {
      if((inb(0x64) & 1) == 1) {
        return;
      }
    }
    return;
  } else {
    while(time_out--) {
      if((inb(0x64) & 2) == 0) {
        return;
      }
    }
    return;
  }
}

/*
 * Write mouse data
 */
void mouse_write(uchar a_write)
{
  /* Wait until able to send a command */
  mouse_wait(MS_WAIT_SIGNAL);
  /* Will send a command */
  outb(0xD4, 0x64);
  /* Wait for the data */
  mouse_wait(MS_WAIT_SIGNAL);
  /* Write */
  outb(a_write, 0x60);
}

/*
 * Read mouse data
 */
uchar mouse_read()
{
  mouse_wait(MS_WAIT_DATA);
  return inb(0x60);
}

extern void install_mouse_IRQ_handler(); /* Need to init mouse */
/*
 * Init mouse
 */
void mouse_init()
{
  uchar status = 0;

  /* Enable the auxiliary mouse device */
  mouse_wait(MS_WAIT_SIGNAL);
  outb(0xA8, 0x64);

  /* Enable mouse interrupts */
  mouse_wait(MS_WAIT_SIGNAL);
  outb(0x20, 0x64);
  mouse_wait(MS_WAIT_DATA);
  status = (inb(0x60)|2);
  mouse_wait(MS_WAIT_SIGNAL);
  outb(0x60, 0x64);
  mouse_wait(MS_WAIT_SIGNAL);
  outb(status, 0x60);

  /* Use default settings */
  mouse_write(0xF6);
  mouse_read();  /* Acknowledge */

  /* Enable mouse */
  mouse_write(0xF4);
  mouse_read();  /* Acknowledge */

  /* Install handler */
  install_mouse_IRQ_handler();
}

/* Read serial port */
#define COM1_PORT 0x3F8
uchar io_in_char_serial()
{
  if(inb(COM1_PORT+5) & 0x01) {
    return inb(COM1_PORT);
  }
  return 0;
}

/*
 * Called each time the system timer advances
 */
void kernel_time_tick()
{
  uint i = 0;

  /* Turn off floppy disk motors. Need to do this
   * manually since some computers use the default
   * PIT handler to control this, but this handler
   * is now being used only for timer purposes */
  for(i=0; i<2; i++) {
    if(disk_info[i].last_access != 0 &&
      system_timer_ms-disk_info[i].last_access > 3000) {
        turn_off_fd_motors();
        debugstr("Turn off floppy disk motors\n\r");
        disk_info[i].last_access = 0;
      i++;
    }
  }

  if(system_timer_ms - cursor_blink_timer > 500) {
    video_blink_cursor();
  }

  return;
}

/*
 * The main kernel function
 */
void kernel()
{
  uint result = 0;
  uint i = 0;
  uint n = 0;

  /* Init heap */
  heap_init();

  /* Init far memory */
  lmem_init();

  /* Init video */
  if(graphics_mode) {
    io_set_graphics_mode();
  } else {
    io_set_text_mode();
  }

  io_show_cursor();
  io_clear_screen();

  /* Setup disk identifiers */
  disk_info[0].id = 0x00; /* Floppy disk 0 */
  strcpy_s(disk_info[0].name, "fd0", sizeof(disk_info[0].name));

  disk_info[1].id = 0x01; /* Floppy disk 1 */
  strcpy_s(disk_info[1].name, "fd1", sizeof(disk_info[1].name));

  disk_info[2].id = 0x80; /* Hard disk 0 */
  strcpy_s(disk_info[2].name, "hd0", sizeof(disk_info[2].name));

  disk_info[3].id = 0x81; /* Hard disk 1 */
  strcpy_s(disk_info[3].name, "hd1", sizeof(disk_info[3].name));

  /* Initialize hardware related disks info */
  debugstr("Disk auxiliar buffer at: %x\n\r", disk_buff);

  for(i=0; i<MAX_DISK; i++) {
    n = index_to_disk(i);

    /* Try to get info */
    result = get_disk_info(n, &disk_info[i].sectors,
      &disk_info[i].sides, &disk_info[i].cylinders);

    /* Use retrieved data if success */
    if(result == 0) {
      disk_info[i].size = ((ul_t)disk_info[i].sectors *
        (ul_t)disk_info[i].sides * (ul_t)disk_info[i].cylinders) /
        (1048576L / (ul_t)SECTOR_SIZE);

      disk_info[i].last_access = system_timer_ms;

      debugstr("DISK (%x : size=%U MB sect_per_track=%d, sides=%d, cylinders=%d)\n\r",
        n, disk_info[i].size, disk_info[i].sectors, disk_info[i].sides,
        disk_info[i].cylinders);

    } else {
      /* Failed. Do not use this disk */
      disk_info[i].sectors = 0;
      disk_info[i].sides = 0;
      disk_info[i].cylinders = 0;
      disk_info[i].size = 0;
      disk_info[i].last_access = 0;
    }
  }

  /* Init FS info */
  fs_init_info();

  /* Init PIC */
  PIC_init();

  /* Init timer at 100 Hz */
  timer_init(100L);

  /* Init mouse */
  mouse_init();

  /* Init PCI */
  pci_init();

  /* Init network */
  net_init();

  /* Execute config file */
  cli_exec_file("config.ini");

  putstr("Starting...\n\r");
  debugstr("Starting...\n\r");

  /* Run intergrated CLI */
  while(1) {
    cli();
  }
  
  /* This code should never be executed */
  /* Halt the computer */
  halt();
}