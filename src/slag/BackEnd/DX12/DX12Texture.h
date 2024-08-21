#ifndef SLAG_DX12TEXTURE_H
#define SLAG_DX12TEXTURE_H

#include <wrl/client.h>
#include <d3d12.h>

namespace slag
{
    namespace dx
    {

        class DX12Texture
        {
        public:

        private:
            Microsoft::WRL::ComPtr<ID3D12Resource> _dxTexture;
        };

    } // dx
} // slag

#endif //SLAG_DX12TEXTURE_H
