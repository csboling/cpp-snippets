#ifndef __RefCounter_HPP_GUARD
#define __RefCounter_HPP_GUARD

namespace utils
{
  class RefCounter
  {
    private:
      unsigned int count;

    public:
      RefCounter()
      : count(0)
      {}

      void acquire
        (void)
      {
        count++;
      }

      unsigned int release
        (void)
      {
        return --count;
      }
  };
}

#endif /* __RefCounter_HPP_GUARD */
