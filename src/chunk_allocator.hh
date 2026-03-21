// The ChunkAllocator is a general purpose memory allocator whose minimum allocation size is 1 KiB.
// It holds a Vector for each allocation size with SegmentLists containing chunks of the 
// corresponding size.
// Chunk sizes double at every step; 1 KiB, 2 KiB, 4 KiB, 8 KiB, etc.

struct ChunkAllocator {
  union Chunk {
    Chunk *next;

  };

  Allocator chunk_allocator;
  Vector<RawSegmentList> chunks;

  void init(Allocator list_allocator, Allocator chunk_allocator);
  void deinit();

  void *alloc(u32 byte_count);
  void dealloc(void *p);
};

void *ChunkAllocator::alloc(u32 byte_count) {
  u32 chunk_size = round_up_to_nearest_power_of_two(max(byte_count, KiB(1)));
  u32 i = chunk_size_index(chunk_size);
  void *chunk = chunks[i].push(chunk_allocator);
  return chunk;
}
