#include "plugins/AnimationPlugin.h"

void AnimationPlugin::setup()
{
    this->step = 0;
    if (customAnimationFrames.size() == 0)
    {
        // Default: 2-frame 8x8 Invader (Crab) centered as 16x16 frames
        const uint8_t crab1[8] = {
          0b00111100,
          0b01111110,
          0b11011011,
          0b11111111,
          0b01111110,
          0b00100100,
          0b01000010,
          0b10000001
        };
        const uint8_t crab2[8] = {
          0b00111100,
          0b01111110,
          0b11011011,
          0b11111111,
          0b01111110,
          0b01000010,
          0b00100100,
          0b00000000
        };

        auto makeFrame = [](const uint8_t rows8[8]){
          std::vector<int> bytes(32, 0);
          for (int y = 0; y < 16; ++y)
          {
            uint8_t leftByte = 0;
            uint8_t rightByte = 0;
            if (y >= 4 && y < 12)
            {
              int dy = y - 4; // 0..7 within sprite
              uint8_t rowMask = rows8[dy];
              for (int dx = 0; dx < 8; ++dx)
              {
                // MSB-left within the 8-bit row mask
                int bitOn = (rowMask >> (7 - dx)) & 1;
                if (!bitOn) continue;
                int x = 4 + dx; // place sprite centered (x 4..11)
                if (x < 8)
                {
                  leftByte |= (1u << (7 - x));
                }
                else
                {
                  rightByte |= (1u << (15 - x)); // == (1 << (7 - (x-8)))
                }
              }
            }
            bytes[y * 2 + 0] = (int)leftByte;
            bytes[y * 2 + 1] = (int)rightByte;
          }
          return bytes;
        };

        customAnimationFrames.push_back(makeFrame(crab1));
        customAnimationFrames.push_back(makeFrame(crab2));
    }
}

void AnimationPlugin::loop()
{
    int size = customAnimationFrames.size();

    if (size > 0)
    {
        std::vector<int> bits = Screen.readBytes(customAnimationFrames[this->step]);

        for (int i = 0; i < bits.size(); i++)
        {
            Screen.setPixelAtIndex(i, bits[i]);
        }

        this->step++;

        if (this->step >= size)
        {
            this->step = 0;
        }
        delay(400);
    }
}

void AnimationPlugin::websocketHook(DynamicJsonDocument &request)
{
    const char *event = request["event"];
    if (!strcmp(event, "upload"))
    {
        int size = (int)request["screens"];

        customAnimationFrames.resize(size);
        for (int i = 0; i < size; i++)
        {
            for (int k = 0; k < 32; k++)
            {
                if (k == 0)
                {
                    customAnimationFrames[i].resize(32);
                }
                customAnimationFrames[i][k] = (int)request["data"][i][k];
            }
        }
    }
}

const std::vector<std::vector<int>> &AnimationPlugin::getFrames() const
{
    return customAnimationFrames;
}


const char *AnimationPlugin::getName() const
{
    return "Animation";
}
