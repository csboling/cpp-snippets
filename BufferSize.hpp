#ifndef __BufferSize_HPP_GUARD
#define __BufferSize_HPP_GUARD

extern "C"
{
  #include <stdint.h>
  #include <stddef.h>
};

const unsigned int BYTES_PER_CHAR = 2;

struct size
{
  uint32_t s;
  explicit size(int x) : s(x) {}
  virtual size_t bytes() const = 0;
};

#define EXPAND_SIZE(name, referent)               \
  struct name##_size : public size                \
  {                                               \
    typedef referent referent_t;                  \
                                                  \
    static const unsigned int bytewidth =         \
      sizeof(referent_t)*BYTES_PER_CHAR;          \
    explicit name##_size(size_t x) : size(x) {}   \
                                                  \
    template <typename SrcT>                      \
    explicit name##_size                          \
      (SrcT& other)                               \
    : size(other.bytes() / bytewidth) {}          \
                                                  \
    size_t bytes() const                          \
    {                                             \
      return s*bytewidth;                         \
    }                                             \
  };
EXPAND_SIZE(byte,  uint16_t)
EXPAND_SIZE(hword, uint16_t)
EXPAND_SIZE(word,  uint32_t)
#undef EXPAND_SIZE

template <typename DstT, typename SrcT, bool DstSmaller>
class _Copier
{
  static void _copy
    (DstT::referent_t* const dst,
     const SrcT::referent_t* const src,
     const SrcT size);
};

template <typename DstT, typename SrcT>
void copy
  (DstT::referent_t* const dst,
   const SrcT::referent_t* const src,
   const SrcT size)
{
  _Copier<
    DstT, SrcT,
    sizeof(SrcT::referent_t) <= sizeof(DstT::referent_t)
  >::_copy(dst, src, size);
}

template <typename DstT, typename SrcT>
class _Copier<DstT, SrcT, true>
{
  friend void utils::copy<DstT, SrcT>
    (DstT::referent_t* const dst,
     const SrcT::referent_t* const src,
     const SrcT size);
  static void _copy
    (DstT::referent_t* const dst,
     const SrcT::referent_t* const src,
     const SrcT size)
  {
    unsigned int dstsize = sizeof(DstT::referent_t);
    unsigned int srcsize = sizeof(SrcT::referent_t);
    for (size_t i = 0; i < size.bytes()/DstT::bytewidth; i++)
    {
      dst[i] = 0;
      for (size_t j = 0;
           j < sizeof(DstT::referent_t)/sizeof(SrcT::referent_t);
           j++)
      {
        dst[i] |= src[i + j] << (j*SrcT::bytewidth*8);
      }
    }
  }
};

template <typename DstT, typename SrcT>
class _Copier<DstT, SrcT, false>
{
  friend void utils::copy<DstT, SrcT>
    (DstT::referent_t* const dst,
     const SrcT::referent_t* const src,
     const SrcT size);
  static void _copy
    (DstT::referent_t* const dst,
     const SrcT::referent_t* const src,
     const SrcT size)
  {
    unsigned long mask = (1UL << DstT::bytewidth*8) - 1;
    size_t bitwidth =  DstT::bytewidth*8;
    size_t total_size = size.bytes()/DstT::bytewidth;
    for (size_t i = 0; i < size.bytes()/SrcT::bytewidth; i++)
    {
      for (size_t j = 0;
           j < sizeof(SrcT::referent_t)/sizeof(DstT::referent_t);
           j++)
      {
        dst[i + j] = (src[i] & (mask << j*bitwidth)) >> j*bitwidth;
      }
    }
  }
};

#endif /* #ifndef __BufferSize_HPP_GUARD */
