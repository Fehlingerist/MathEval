#include <new>
#include <memory>
#include <cstdint>
#include <cstddef>

namespace Util {
    using byte = std::byte;

    class LinearMemoryManager {
        public:
        enum class ErrorCode{ 
            None,
            NotEnoughMemory
        };

       ErrorCode last_error = ErrorCode::None;

        byte* memory_pointer = nullptr;
        size_t size = 0;
        size_t byte_offset = 0;

        inline bool is_out_of_bounds(size_t offset,size_t t_size)
        {
            return offset > size || offset + t_size > size;
        };

    inline size_t find_first_aligning_address(size_t type_size)
    {
        size_t current_absolute_address = (size_t)memory_pointer + byte_offset;
        size_t remainder = current_absolute_address % type_size;
    
        if (remainder == 0) return byte_offset;

        size_t padding = type_size - remainder;
        return byte_offset + padding;
    }

        inline size_t move_data(byte* new_block)
        {   
            memcpy(new_block,memory_pointer,size); 
            delete[] memory_pointer;
            memory_pointer = new_block;
        };  

        inline size_t resize(size_t new_size)
        {
            try
            {
                auto new_block = new byte[new_size];
                if (new_block == nullptr)
                {
                    last_error = ErrorCode::NotEnoughMemory;
                    return;
                };
                move_data(new_block);
            } catch(std::bad_alloc exception)
            {
                memory_pointer = nullptr;
                last_error = ErrorCode::NotEnoughMemory;
            };
        };

        template <typename T>
        inline size_t allocate(T var)
        {   
            auto t_size = alignof(T);
            auto t_address = find_first_aligning_address(t_size);
            if (is_out_of_bounds(t_address,t_size))
            {
                resize(size * 5);
            };
            byte_offset = t_address;
            return t_address;
        };

        template <typename T = byte>
        inline T& operator[](size_t offset) {
            return *reinterpret_cast<T*>(memory_pointer + offset);
        }

        template <typename T = byte>
        inline const T& operator[](size_t offset) const {
            return *reinterpret_cast<const T*>(memory_pointer + offset);
        }

        LinearMemoryManager(size_t size)
        {
            resize(size);
        };
       
        ~LinearMemoryManager()
        {
            delete[] memory_pointer;
        };
    };

}