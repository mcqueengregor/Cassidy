#include "Mesh.h"
#include <Utils/Initialisers.h>

void cassidy::Mesh::release(VmaAllocator allocator)
{
  vmaDestroyBuffer(allocator, m_vertexBuffer.buffer, m_vertexBuffer.allocation);
}

void cassidy::Mesh::allocateVertexBuffer(VkCommandBuffer uploadCmd, VmaAllocator allocator)
{
  // Build CPU-side staging buffer:
  VkBufferCreateInfo stagingBufferInfo = cassidy::init::bufferCreateInfo(m_vertices.size() * sizeof(Vertex),
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  VmaAllocationCreateInfo bufferAllocInfo = cassidy::init::vmaAllocationCreateInfo(VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  AllocatedBuffer stagingBuffer;

  vmaCreateBuffer(allocator, &stagingBufferInfo, &bufferAllocInfo,
    &stagingBuffer.buffer,
    &stagingBuffer.allocation,
    nullptr);

  // Write vertex data to newly-allocated buffer:
  void* data;
  vmaMapMemory(allocator, stagingBuffer.allocation, &data);
  memcpy(data, m_vertices.data(), m_vertices.size() * sizeof(Vertex));
  vmaUnmapMemory(allocator, stagingBuffer.allocation);

  VkBufferCreateInfo vertexBufferInfo = cassidy::init::bufferCreateInfo(m_vertices.size() * sizeof(Vertex),
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

  bufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  AllocatedBuffer newBuffer;

  vmaCreateBuffer(allocator, &vertexBufferInfo, &bufferAllocInfo,
    &newBuffer.buffer,
    &newBuffer.allocation,
    nullptr);

  // Execute copy command for CPU-side staging buffer -> GPU-side vertex buffer:
  uploadBuffer([=](VkCommandBuffer cmd) {
    VkBufferCopy copy = {};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = m_vertices.size() * sizeof(Vertex);
    vkCmdCopyBuffer(cmd, stagingBuffer.buffer, newBuffer.buffer, 1, &copy);
  });

  vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}
