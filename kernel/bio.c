// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache;

struct {
  struct buf buckets[BUCKETS];
  struct spinlock locks[BUCKETS];
} htable;

int
hash(uint blockno)
{
  return blockno % BUCKETS;
}

void
hlock(uint no)
{
  acquire(&htable.locks[hash(no)]);
}

void
hrelease(uint no)
{
  release(&htable.locks[hash(no)]);
}

struct buf*
_search(uint blockno)
{
  struct buf *head = &htable.buckets[hash(blockno)];
  for (struct buf *node=head->next; node != head; node=node->next) {
    if (node->blockno == blockno) {
      return node;
    }
  }
  return 0;
}

struct buf*
search(uint blockno)
{
  hlock(blockno);
  struct buf *node = _search(blockno);
  hrelease(blockno);
  return node;
}

void
insert(struct buf *new_b)
{
  uint blockno = new_b->blockno;
  hlock(blockno);
  struct buf *b, *head;
  b = head = &htable.buckets[hash(blockno)];

  while (b->next != head) {
    b = b->next;
  }
  b->next->prev = new_b;
  new_b->next = b->next;
  b->next = new_b;
  new_b->prev = b;
  hrelease(blockno);
}

void
remove(uint blockno){
  hlock(blockno);
  struct buf *b;
  b = _search(blockno);
  if (b) {
    b->prev->next = b->next;
    b->next->prev = b->prev;
  }
  hrelease(blockno);
}

void
binit(void)
{


  initlock(&bcache.lock, "bcache_main");
  struct buf *b;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    initlock(&b->splock, "buffer_splock");
  }
  for(int i=0; i < BUCKETS; i++){
    int size = 8 + 1;
    char name[size];
    snprintf(name, size, "bcache_%d", i);
    initlock(&htable.locks[i], name);
    b = &htable.buckets[i];
    b->prev = b;
    b->next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  b = search(blockno);
  if (b) {
    acquire(&b->splock);
    b->refcnt++;
    release(&b->splock);
    acquiresleep(&b->lock);
    return b;
  }

  b = bcache.buf;
  uint min = -1U;
  struct buf *lru_b = 0;
  acquire(&bcache.lock);
  for (int i = 0; i < NBUF; i++) {
    acquire(&b->splock);
    if (!b->refcnt && b->last_timestamp < min){
      min = b->last_timestamp;
      lru_b = b;
    }
    release(&b->splock);
    b++;
  }

  if (lru_b){
    remove(lru_b->blockno);
    acquire(&lru_b->splock);
    lru_b->last_timestamp = ticks;

    lru_b->dev = dev;
    lru_b->blockno = blockno;
    lru_b->valid = 0;
    lru_b->refcnt = 1;
    release(&lru_b->splock);
    release(&bcache.lock);

    insert(lru_b);

    acquiresleep(&lru_b->lock);
    return lru_b;
  }
  release(&bcache.lock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&b->splock);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->last_timestamp = ticks;
  }
  release(&b->splock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


