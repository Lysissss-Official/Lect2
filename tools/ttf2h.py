# ttf_to_h.py
from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont
import numpy as np
import os

def ttf_to_header(ttf_path, output_path, font_size=16, chars=None):
    """
    将TTF字体转换为C头文件
    """
    if chars is None:
        # 默认包含ASCII和常用中文
        chars = "".join([chr(i) for i in range(32, 127)])# +
        #[chr(i) for i in range(0x4E00, 0x4E50)])  # 部分中文

    font = ImageFont.truetype(ttf_path, font_size)

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(f"#ifndef FONT_{font_size}_H\n")
        f.write(f"#define FONT_{font_size}_H\n\n")
        f.write("#include <stdint.h>\n")
        f.write("#include \"FontBase.h\"\n\n")

        f.write(f"// Font: {os.path.basename(ttf_path)}\n")
        f.write(f"// Size: {font_size}px\n")
        f.write(f"// Characters: {len(chars)}\n\n")

        f.write(f"class Font_{font_size} : public FontBase\n")
        f.write("{using FontBase::FontBase;};\n\n")

        f.write(f"Font_{font_size} f{font_size}(FONT_SYSTEM,\n")
        f.write("{\n")

        char_data = []

        for char in chars:
            # 获取字符尺寸
            bbox = font.getbbox(char)
            width = bbox[2] - bbox[0] if bbox[2] > bbox[0] else font_size
            height = bbox[3] - bbox[1] if bbox[3] > bbox[1] else font_size

            # 创建图像并绘制字符
            img = Image.new('1', (width, height), 0)
            draw = ImageDraw.Draw(img)
            draw.text((-bbox[0], -bbox[1]), char, font=font, fill=1)

            # 转换为字节数组
            pixels = np.array(img).flatten()
            bytes_required = (len(pixels) + 7) // 8
            byte_array = []

            for i in range(bytes_required):
                byte_val = 0
                for bit in range(8):
                    if i * 8 + bit < len(pixels) and pixels[i * 8 + bit]:
                        byte_val |= (1 << (7 - bit))
                byte_array.append(byte_val)

            # 生成变量名
            var_name = f"font_{ord(char):04X}"
            f.write(f"/*\t{char}\t*/")
            f.write("{")
            f.write(f"0x{ord(char):04X}, {width}, {height}, ")
            f.write("{")
            f.write(",".join([f"0x{b:02X}" for b in byte_array]))
            f.write("}},\n")

            char_data.append((ord(char), width, height, var_name))

        f.write("/*\t###\t*/")
        f.write("{0x0000, 0, 0, {}}\n")
        f.write("});\n\n")
        f.write(f"#define FONT_{font_size}_COUNT {len(char_data)}\n")
        f.write(f"#endif // FONT_{font_size}_H\n")


ttf_to_header("/Users/niumo/Downloads/Avenir Next Condensed.ttf",
              "font_48.h", 48)