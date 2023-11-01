#ifndef SLAG_DX12TEXTURE_H
#define SLAG_DX12TEXTURE_H
#include "../../Texture.h"
#include "../Resource.h"
#include <d3d12.h>

namespace slag
{
    namespace dx12
    {

        class DX12Texture: public Texture, Resource
        {
        public:
            ~DX12Texture()override;
            DX12Texture(DX12Texture&)=delete;
            DX12Texture& operator=(DX12Texture&)=delete;
            DX12Texture(DX12Texture&& from);
            DX12Texture& operator=(DX12Texture&& from);
        private:
            void move(DX12Texture&& from);
        };

    } // slag
} // dx12

#endif //SLAG_DX12TEXTURE_H
