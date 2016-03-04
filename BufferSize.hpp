#ifndef __address_HPP_GUARD
#define __address_HPP_GUARD

#include <iterator>

#include "utils/memory/RefCounter.hpp"

extern "C"
{
  #include <stdint.h>
  #include <stddef.h>
}

const unsigned int BYTES_PER_CHAR = 2;


/* These classes deliberately do not define (non-explicit) conversion
 * or assignment operators -- we want the user to be explicit about
 * changing between size and buffer types so that inadvertent
 * conversions are compiler errors.
 */

template <typename Derived> struct size_traits;

template <typename Derived>
struct size : public size_traits<Derived>
{
  typedef typename size_traits<Derived>::referent_t referent_t;

  uint32_t s;
  size() : s(0) {}

  explicit size
    (int x)
  : s(x)
  {}

  virtual size_t bytes() const = 0;
};

#define EXPAND_SIZE(name, referent)                                   \
  struct name##_size;                                                 \
  template <> struct size_traits<name##_size>                         \
  {                                                                   \
    typedef referent referent_t;                                      \
    static const unsigned int bytewidth =                             \
      sizeof(referent_t)*BYTES_PER_CHAR;                              \
  };                                                                  \
                                                                      \
  struct name##_size : public size<name##_size>                       \
  {                                                                   \
    typedef size_traits<name##_size>::referent_t referent_t;          \
                                                                      \
    name##_size() : size<name##_size>(0) {}                           \
                                                                      \
    explicit name##_size                                              \
      (int x)                                                         \
    : size(x)                                                         \
    {}                                                                \
                                                                      \
    template <typename SrcT>                                          \
    explicit name##_size                                              \
      (const size<SrcT>& other)                                       \
    : size<name##_size>(other.bytes() / bytewidth)                    \
    {}                                                                \
                                                                      \
    size_t bytes() const                                              \
    {                                                                 \
      return s*bytewidth;                                             \
    }                                                                 \
  };
EXPAND_SIZE(byte,  uint16_t)
EXPAND_SIZE(hword, uint16_t)
EXPAND_SIZE(word,  uint32_t)
#undef EXPAND_SIZE

template <typename Size>
struct Buffer
{
  template <typename OtherSize>
  friend class Buffer;

  Size::referent_t* p;
  Size s;

  Buffer()
  : p(NULL)
  , s(Size(0))
  , refs(new utils::RefCounter())
  , dynamic(false)
  {
    refs->acquire();
  }

  Buffer
    (const Size& s)
  : p(new Size::referent_t[s.s])
  , s(s)
  , refs(new utils::RefCounter())
  , dynamic(true)
  {
    refs->acquire();
    std::memset(
      p,
      0,
      s.bytes() / BYTES_PER_CHAR
    );
  }

  Buffer
    (Size::referent_t* const p, const Size& s)
  : p(p)
  , s(s)
  , dynamic(false)
  , refs(new utils::RefCounter())
  {
    refs->acquire();
  }

  /* both copy constructors are required? */
  Buffer
    (const Buffer& other)
  : p(other.p)
  , s(other.s)
  , refs(other.refs)
  , dynamic(other.dynamic)
  {
    refs->acquire();
  }

  template <typename OtherSize>
  Buffer
    (const Buffer<OtherSize>& other)
  : p((Size::referent_t*)other.p)
  , s(other.s)
  , refs(other.refs)
  , dynamic(other.dynamic)
  {
    refs->acquire();
  }

  ~Buffer()
  {
    if (refs->release() == 0)
    {
      if (dynamic)
      {
        delete[] p;
      }
      delete refs;
    }
  }

  private:
    utils::RefCounter* refs;
    bool dynamic;
};

template <typename Size, unsigned int Depth>
class FixedBuffer : public Buffer<Size>
{
  public:
    FixedBuffer()
    : Buffer<Size>(contents, Size(Depth))
    {}

    template <typename SrcT>
    FixedBuffer(Buffer<SrcT>& other)
    : Buffer<Size>(
        contents,
		Size(std::min(sizeof(contents)*BYTES_PER_CHAR, other.s.bytes()) / Size::bytewidth)
      )
    {
      utils::copy<Size, SrcT>(contents, other.p, s);
    }

  private:
    Size::referent_t contents[Depth];
};

namespace utils
{
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
  void copy
    (Buffer<DstT>& dst, Buffer<SrcT>& src)
  {
    copy<DstT, SrcT>(
      dst.p,
      src.p,
      DstT(std::min(dst.bytes(), src.bytes()) / DstT::bytewidth)
    );
    dst.s = DstT(src.s);
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
}

#endif /* #ifndef __address_HPP_GUARD */
