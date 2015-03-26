#include "basic_types.hpp"

#include "../core/utilities.hpp"

namespace panoramix {
    namespace gui {

        using namespace core;

        inline Color ColorFromTag(ColorTag t) {
            switch (t){
            case ColorTag::Transparent: return Color(0, 0, 0, 0);

            case ColorTag::White: return Color(255, 255, 255);
            case ColorTag::Black: return Color(0, 0, 0);

            case ColorTag::DimGray: return Color(105, 105, 105);
            case ColorTag::Gray: return Color(128, 128, 128);
            case ColorTag::DarkGray: return Color(169, 169, 169);
            case ColorTag::Silver: return Color(192, 192, 192);
            case ColorTag::LightGray: return Color(211, 211, 211);

            case ColorTag::Red: return Color(255, 0, 0);
            case ColorTag::Green: return Color(0, 255, 0);
            case ColorTag::Blue: return Color(0, 0, 255);

            case ColorTag::Yellow: return Color(255, 255, 0);
            case ColorTag::Magenta: return Color(255, 0, 255);
            case ColorTag::Cyan: return Color(0, 255, 255);
            case ColorTag::Orange: return Color(255, 165, 0);
            default:
                return Color(255, 255, 255);
            }
        }


        Color::Color(ColorTag tag) : _rgba(ColorFromTag(tag)._rgba) {}
        Color::Color(const std::uint8_t * data, int cvType) {
            const int16_t * datai16 = (const int16_t*)(data);
            const int32_t * datai32 = (const int32_t*)(data);
            const float * dataf32 = (const float *)(data);
            const double * dataf64 = (const double *)(data);

            switch (cvType){
            case CV_8UC1: _rgba = Vec4i(data[0], data[0], data[0], 255); break;
            case CV_8UC3: _rgba = Vec4i(data[0], data[1], data[2], 255); break;
            case CV_8UC4: _rgba = Vec4i(data[0], data[1], data[2], data[3]); break;

            case CV_16SC1: _rgba = Vec4i(datai16[0], datai16[0], datai16[0], 255); break;
            case CV_16SC3: _rgba = Vec4i(datai16[0], datai16[1], datai16[2], 255); break;
            case CV_16SC4: _rgba = Vec4i(datai16[0], datai16[1], datai16[2], datai16[3]); break;

            case CV_32SC1: _rgba = Vec4i(datai32[0], datai32[0], datai32[0], 255); break;
            case CV_32SC3: _rgba = Vec4i(datai32[0], datai32[1], datai32[2], 255); break;
            case CV_32SC4: _rgba = Vec4i(datai32[0], datai32[1], datai32[2], datai32[3]); break;

            case CV_32FC1: _rgba = Vec4i(dataf32[0], dataf32[0], dataf32[0], 1) * 255; break;
            case CV_32FC3: _rgba = Vec4i(dataf32[0], dataf32[1], dataf32[2], 1) * 255; break;
            case CV_32FC4: _rgba = Vec4i(dataf32[0], dataf32[1], dataf32[2], dataf32[3]) * 255; break;

            case CV_64FC1: _rgba = Vec4i(dataf64[0], dataf64[0], dataf64[0], 1) * 255; break;
            case CV_64FC3: _rgba = Vec4i(dataf64[0], dataf64[1], dataf64[2], 1) * 255; break;
            case CV_64FC4: _rgba = Vec4i(dataf64[0], dataf64[1], dataf64[2], dataf64[3]) * 255; break;
            default:
                std::cerr << "cannot convert this cv type to vis::Color [cvType = " << cvType << "]!" << std::endl;
                break;
            }
        }

        const std::vector<ColorTag> & AllColorTags() {
            static const std::vector<ColorTag> _allColorTags = {
                ColorTag::Transparent,
                ColorTag::White,
                ColorTag::Gray,
                ColorTag::Red,
                ColorTag::Green,
                ColorTag::Blue,
                ColorTag::Yellow,
                ColorTag::Magenta,
                ColorTag::Cyan,
                ColorTag::Orange
            };
            return _allColorTags;
        }

        std::ostream & operator << (std::ostream & os, ColorTag ct) {
            switch (ct){
            case ColorTag::Transparent: os << "Transparent"; break;

            case ColorTag::White: os << "White"; break;
            case ColorTag::Black: os << "Black"; break;

            case ColorTag::DimGray: os << "DimGray"; break;
            case ColorTag::Gray: os << "Gray"; break;
            case ColorTag::DarkGray: os << "DarkGray"; break;
            case ColorTag::Silver: os << "Silver"; break;
            case ColorTag::LightGray: os << "LightGray"; break;

            case ColorTag::Red: os << "Red"; break;
            case ColorTag::Green: os << "Green"; break;
            case ColorTag::Blue: os << "Blue"; break;

            case ColorTag::Yellow: os << "Yellow"; break;
            case ColorTag::Magenta: os << "Magenta"; break;
            case ColorTag::Cyan: os << "Cyan"; break;
            case ColorTag::Orange: os << "Orange"; break;
            default:
                os << "Unknown Color"; break;
            }

            return os;
        }

        Color ColorFromHSV(double h, double s, double v, double a) {
            int i;
            double f, p, q, t;
            if (s == 0.0) {
                // achromatic (grey)
                return Color(v, v, v, a);
            }
            h *= 6.0;			// sector 0 to 5
            i = floor(h);
            f = h - i;			// factorial part of h
            p = v * (1 - s);
            q = v * (1 - s * f);
            t = v * (1 - s * (1 - f));
            switch (i) {
            case 0: return Color(v, t, p, a);
            case 1: return Color(q, v, p, a);
            case 2: return Color(p, v, t, a);
            case 3: return Color(p, q, v, a);
            case 4: return Color(t, p, v, a);
            default:return Color(v, p, q, a);
            }
        }

        Color RandomColor() {
            return Color(rand() % 255, rand() % 255, rand() % 255);
        }


        ColorTable::ColorTable(ColorTableDescriptor descriptor) {
            const auto & predefined = PredefinedColorTable(descriptor);
            _colors = predefined._colors;
            _exceptionalColor = predefined._exceptionalColor;
        }
        ColorTable::ColorTable(std::initializer_list<ColorTag> ctags, ColorTag exceptColor) {
            _colors.reserve(ctags.size());
            for (auto ct : ctags) {
                _colors.push_back(ColorFromTag(ct));
            }
            _exceptionalColor = ColorFromTag(exceptColor);
        }

        core::Imageub3 ColorTable::operator()(const core::Imagei & indexIm) const{
            core::Imageub3 im(indexIm.size());
            for (auto i = indexIm.begin(); i != indexIm.end(); ++i){
                core::Vec3b color = (*this)[*i];
                im(i.pos()) = color;
            }
            return im;
        }

        ColorTable & ColorTable::randomize() { std::random_shuffle(_colors.begin(), _colors.end()); return *this; }

        ColorTable & ColorTable::appendRandomizedColors(size_t sz) {
            int dimSplit = std::max(int(sqrt(sz)), 3);
            std::vector<Color> colors;
            colors.reserve(dimSplit * dimSplit * dimSplit - dimSplit);
            for (int i = 0; i < dimSplit; i++){
                for (int j = 0; j < dimSplit; j++){
                    for (int k = 0; k < dimSplit; k++){
                        if (i == j && j == k)
                            continue;
                        colors.push_back(Color(1.0 * i / dimSplit, 1.0 * j / dimSplit, 1.0 * k / dimSplit));
                    }
                }
            }
            assert(colors.size() > sz);
            std::random_shuffle(colors.begin(), colors.end());
            _colors.insert(_colors.end(), colors.begin(), colors.begin() + sz);
            return *this;
        }

        ColorTable & ColorTable::appendRandomizedGreyColors(size_t size) {
            core::Vec3 full(255, 255, 255);
            std::vector<Color> colors(size);
            for (int i = 0; i < size; i++){
                colors[i] = Color(double(i) * full / double(size));
            }
            std::random_shuffle(colors.begin(), colors.end());
            _colors.insert(_colors.end(), colors.begin(), colors.end());
            return *this;
        }

        const ColorTable & PredefinedColorTable(ColorTableDescriptor descriptor) {
           
            static const ColorTable allColorTable = {
                {
                    ColorTag::White,
                    ColorTag::Black,

                    ColorTag::DimGray,
                    ColorTag::Gray,
                    ColorTag::DarkGray,
                    ColorTag::Silver,
                    ColorTag::LightGray,

                    ColorTag::Red,
                    ColorTag::Green,
                    ColorTag::Blue,

                    ColorTag::Yellow,
                    ColorTag::Magenta,
                    ColorTag::Cyan,
                    ColorTag::Orange
                }, 
                ColorTag::Transparent
            };

            static const ColorTable allColorExcludingWhiteTable = {
                {
                    //ColorTag::White,
                    ColorTag::Black,

                    ColorTag::DimGray,
                    ColorTag::Gray,
                    ColorTag::DarkGray,
                    ColorTag::Silver,
                    ColorTag::LightGray,

                    ColorTag::Red,
                    ColorTag::Green,
                    ColorTag::Blue,

                    ColorTag::Yellow,
                    ColorTag::Magenta,
                    ColorTag::Cyan,
                    ColorTag::Orange
                },
                ColorTag::Transparent
            };

            static const ColorTable allColorExcludingBlackTable = {
                {
                    ColorTag::White,
                    //ColorTag::Black,

                    ColorTag::DimGray,
                    ColorTag::Gray,
                    ColorTag::DarkGray,
                    ColorTag::Silver,
                    ColorTag::LightGray,

                    ColorTag::Red,
                    ColorTag::Green,
                    ColorTag::Blue,

                    ColorTag::Yellow,
                    ColorTag::Magenta,
                    ColorTag::Cyan,
                    ColorTag::Orange
                },
                ColorTag::Transparent
            };

            static const ColorTable allColorExcludingWhiteAndBlackTable = {
                {
                    //ColorTag::White,
                    //ColorTag::Black,

                    ColorTag::DimGray,
                    ColorTag::Gray,
                    ColorTag::DarkGray,
                    ColorTag::Silver,
                    ColorTag::LightGray,

                    ColorTag::Red,
                    ColorTag::Green,
                    ColorTag::Blue,

                    ColorTag::Yellow,
                    ColorTag::Magenta,
                    ColorTag::Cyan,
                    ColorTag::Orange
                },
                ColorTag::Transparent
            };

            static const ColorTable RGBColorTable = {
                {
                    ColorTag::Red,
                    ColorTag::Green,
                    ColorTag::Blue
                },
                ColorTag::White
            };

            static const ColorTable RGBGreysColorTable = {
                {
                    ColorTag::Red,
                    ColorTag::Green,
                    ColorTag::Blue,

                    ColorTag::DimGray,
                    ColorTag::Gray,
                    ColorTag::DarkGray,
                    ColorTag::Silver,
                    ColorTag::LightGray
                },
                ColorTag::White
            };

            switch (descriptor) {
            case ColorTableDescriptor::RGB: return RGBColorTable;
            case ColorTableDescriptor::AllColorsExcludingBlack: return allColorExcludingBlackTable;
            case ColorTableDescriptor::AllColorsExcludingWhite: return allColorExcludingWhiteTable;
            case ColorTableDescriptor::AllColorsExcludingWhiteAndBlack: return allColorExcludingWhiteAndBlackTable;
            case ColorTableDescriptor::AllColors: return allColorTable;
            default: return RGBGreysColorTable;
            }
            
        }

        ColorTable CreateGreyColorTableWithSize(int sz) {
            auto exeptColor = ColorFromTag(ColorTag::Blue);
            core::Vec3 full(255, 255, 255);
            std::vector<Color> colors(sz);
            for (int i = 0; i < sz; i++){
                colors[i] = Color(double(i) * full / double(sz));
            }
            return ColorTable(colors, exeptColor);
        }

        ColorTable CreateRandomColorTableWithSize(int sz, const Color & exceptColor) {
            int dimSplit = std::max(int(sqrt(sz)), 3);
            std::vector<Color> colors;
            colors.reserve(dimSplit * dimSplit * dimSplit - dimSplit);
            for (int i = 0; i < dimSplit; i++){
                for (int j = 0; j < dimSplit; j++){
                    for (int k = 0; k < dimSplit; k++){
                        if (i == j && j == k)
                            continue;
                        colors.push_back(Color(1.0 * i / dimSplit, 1.0 * j / dimSplit, 1.0 * k / dimSplit));
                    }
                }
            }
            assert(colors.size() > sz);
            std::random_shuffle(colors.begin(), colors.end());
            return ColorTable(colors.begin(), colors.begin() + sz, exceptColor);
        }


        OpenGLShaderSource::OpenGLShaderSource(OpenGLShaderSourceDescriptor d) {
            auto & ss = PredefinedShaderSource(d);
            _vshaderSrc = ss._vshaderSrc;
            _fshaderSrc = ss._fshaderSrc;
        }


        // opengl shader source 
        const OpenGLShaderSource & PredefinedShaderSource(OpenGLShaderSourceDescriptor name) {

            static const OpenGLShaderSource defaultPointsShaderSource = {
                "#version 120\n"
                "attribute highp vec4 position;\n"
                "attribute highp vec3 normal;\n"
                "attribute lowp vec4 color;\n"
                "attribute lowp vec2 texCoord;\n"
                "uniform highp mat4 matrix;\n"
                "uniform float pointSize;\n"
                "varying vec4 pixelColor;\n"
                "void main(void)\n"
                "{\n"
                "    gl_Position = matrix * position;\n"
                "    gl_PointSize = pointSize;\n"
                "    pixelColor = color;\n"
                "}\n",

                "#version 120\n"
                "varying lowp vec4 pixelColor;\n"
                "void main(void)\n"
                "{\n"
                "   gl_FragColor = pixelColor;\n"
                "   float distance = length(gl_PointCoord - vec2(0.5));\n"
                "   if(distance > 0.4 && distance <= 0.5)\n"
                "       gl_FragColor.a = 1.0 - (distance - 0.4) * 0.1;\n"
                "   else if(distance > 0.5)\n"
                "       discard;\n"
                "}\n"
            };

            static const OpenGLShaderSource defaultLinesShaderSource = {
                "#version 120\n"
                "attribute lowp vec4 position;\n"
                "attribute lowp vec3 normal;\n"
                "attribute lowp vec4 color;\n"
                "attribute lowp vec2 texCoord;\n"
                "uniform highp mat4 matrix;\n"
                "uniform float pointSize;\n"
                "varying vec4 pixelColor;\n"
                "void main(void)\n"
                "{\n"
                "    gl_Position = matrix * position;\n"
                "    pixelColor = color;\n"
                "}\n",

                "#version 120\n"
                "varying lowp vec4 pixelColor;\n"
                "void main(void)\n"
                "{\n"
                "    gl_FragColor = pixelColor;\n"
                "}\n"
            };

            static const OpenGLShaderSource defaultTrianglesShaderSource = {
                "#version 120\n"
                "attribute highp vec4 position;\n"
                "attribute highp vec3 normal;\n"
                "attribute lowp vec4 color;\n"
                "attribute lowp vec2 texCoord;\n"
                "uniform highp mat4 matrix;\n"
                "uniform float pointSize;\n"
                "varying vec4 pixelColor;\n"
                "void main(void)\n"
                "{\n"
                "    gl_Position = matrix * position;\n"
                "    highp vec4 transformedNormal = viewMatrix * modelMatrix * vec4(normal, 1.0);\n"
                "    highp vec3 transformedNormal3 = transformedNormal.xyz / transformedNormal.w;\n"
                "    pixelColor = abs(dot(transformedNormal3 / length(transformedNormal), vec3(1.0, 0.0, 0.0))) * vec4(1.0, 1.0, 1.0, 1.0);\n"
                "}\n",

                "#version 120\n"
                "varying lowp vec4 pixelColor;\n"
                "void main(void)\n"
                "{\n"
                "    gl_FragColor = pixelColor;\n"
                "}\n"
            };

            static const OpenGLShaderSource panoramaShaderSource = {
                "#version 120\n"
                "attribute highp vec3 position;\n"
                "attribute highp vec3 normal;\n"
                "attribute highp vec4 color;\n"
                "uniform highp mat4 matrix;\n"
                "varying highp vec3 pixelPosition;\n"
                "varying highp vec3 pixelNormal;\n"
                "varying highp vec4 pixelColor;\n"
                "void main(void)\n"
                "{\n"
                "    pixelPosition = position.xyz;\n"
                "    pixelNormal = normal;\n"
                "    pixelColor = color;\n"
                "    gl_Position = matrix * vec4(position, 1.0);\n"
                "}\n"
                ,

                // 3.14159265358979323846264338327950288
                "uniform sampler2D tex;\n"
                "uniform highp vec3 panoramaCenter;\n"
                "varying highp vec3 pixelPosition;\n"
                "varying highp vec3 pixelNormal;\n"
                "varying highp vec4 pixelColor;\n"
                "void main(void)\n"
                "{\n"
                "    highp vec3 direction = pixelPosition - panoramaCenter;\n"
                "    highp float longi = atan(direction.y, direction.x);\n"
                "    highp float lati = asin(direction.z / length(direction));\n"
                "    highp vec2 texCoord = vec2(longi / 3.1415926535897932 / 2.0 + 0.5, - lati / 3.1415926535897932 + 0.5);\n"
                "    gl_FragColor = texture2D(tex, texCoord) * 1.0 + pixelColor * 0.0;\n"
                "    gl_FragColor.a = 0.7;\n"
                "}\n"
            };








            static const OpenGLShaderSource xPointsShaderSource = {
                "#version 130\n"

                "attribute highp vec4 position;\n"
                "attribute highp vec3 normal;\n"
                "attribute lowp vec4 color;\n"
                "attribute lowp vec2 texCoord;\n"

                "uniform highp mat4 viewMatrix;\n"
                "uniform highp mat4 modelMatrix;\n"
                "uniform highp mat4 projectionMatrix;\n"

                "varying lowp vec4 pixelColor;\n"
                "varying lowp vec2 pixelTexCoord;\n"

                "void main(void)\n"
                "{\n"
                "    gl_Position = projectionMatrix * viewMatrix * modelMatrix * position;\n"
                "    pixelColor = color;\n"
                "    pixelTexCoord = texCoord;\n"
                "}\n",



                "#version 130\n"

                "uniform sampler2D tex;\n"
                "uniform lowp vec4 globalColor;\n"

                "uniform lowp float bwColor;\n"
                "uniform lowp float bwTexColor;\n"

                "varying lowp vec4 pixelColor;\n"
                "varying lowp vec2 pixelTexCoord;\n"

                "void main(void)\n"
                "{\n"
                "   lowp vec4 texColor = texture2D(tex, pixelTexCoord);\n"
                "   gl_FragColor = (pixelColor * bwColor + texColor * bwTexColor)" 
                        "       / (bwColor + bwTexColor);\n"
                "   float distance = length(gl_PointCoord - vec2(0.5));\n"
                "   if(distance > 0.4 && distance <= 0.5)\n"
                "       gl_FragColor.a = 1.0 - (distance - 0.4) * 0.1;\n"
                "   else if(distance > 0.5)\n"
                "       discard;\n"
                "}\n"
            };

            static const OpenGLShaderSource xLinesShaderSource = {
                "#version 130\n"

                "attribute highp vec4 position;\n"
                "attribute highp vec3 normal;\n"
                "attribute lowp vec4 color;\n"
                "attribute lowp vec2 texCoord;\n"

                "uniform highp mat4 viewMatrix;\n"
                "uniform highp mat4 modelMatrix;\n"
                "uniform highp mat4 projectionMatrix;\n"

                "varying lowp vec4 pixelColor;\n"
                "varying lowp vec2 pixelTexCoord;\n"

                "void main(void)\n"
                "{\n"
                "    gl_Position = projectionMatrix * viewMatrix * modelMatrix * position;\n"
                "    pixelColor = color;\n"
                "    pixelTexCoord = texCoord;\n"
                "}\n",



                "#version 130\n"

                "uniform sampler2D tex;\n"
                "uniform lowp vec4 globalColor;\n"

                "uniform lowp float bwColor;\n"
                "uniform lowp float bwTexColor;\n"

                "varying lowp vec4 pixelColor;\n"
                "varying lowp vec2 pixelTexCoord;\n"

                "void main(void)\n"
                "{\n"
                "   lowp vec4 texColor = texture2D(tex, pixelTexCoord);\n"
                "   gl_FragColor = (pixelColor * bwColor + texColor * bwTexColor)"
                "       / (bwColor + bwTexColor);\n"
                "}\n"
            };

            static const OpenGLShaderSource xTrianglesShaderSource = {
                "#version 130\n"

                "attribute highp vec4 position;\n"
                "attribute highp vec3 normal;\n"
                "attribute lowp vec4 color;\n"
                "attribute lowp vec2 texCoord;\n"
                "attribute uint isSelected;\n"

                "uniform highp mat4 viewMatrix;\n"
                "uniform highp mat4 modelMatrix;\n"
                "uniform highp mat4 projectionMatrix;\n"

                "varying lowp vec4 pixelColor;\n"
                "varying lowp vec2 pixelTexCoord;\n"
                "varying lowp float pixelSelection;\n"

                "void main(void)\n"
                "{\n"
                "    gl_Position = projectionMatrix * viewMatrix * modelMatrix * position;\n"
                "    pixelColor = color;\n"
                "    pixelTexCoord = texCoord;\n"
                "    pixelSelection = isSelected == 0u ? 0.0 : 1.0;\n"
                "}\n",



                "#version 130\n"

                "uniform sampler2D tex;\n"
                "uniform lowp vec4 globalColor;\n"

                "uniform lowp float bwColor;\n"
                "uniform lowp float bwTexColor;\n"

                "varying lowp vec4 pixelColor;\n"
                "varying lowp vec2 pixelTexCoord;\n"
                "varying lowp float pixelSelection;\n"

                "void main(void)\n"
                "{\n"
                "    lowp vec4 texColor = texture2D(tex, pixelTexCoord);\n"
                "    gl_FragColor = (pixelColor * bwColor + texColor * bwTexColor)"
                "       / (bwColor + bwTexColor);\n"
                "    gl_FragColor.a = 1.0 - pixelSelection * 0.5;\n"
                "}\n"
            };

            static const OpenGLShaderSource xPanoramaShaderSource = {
                "#version 130\n"

                "attribute highp vec3 position;\n"
                "attribute highp vec3 normal;\n"
                "attribute highp vec4 color;\n"
                "attribute lowp vec2 texCoord;\n"
                "attribute uint isSelected;\n"

                "uniform highp mat4 viewMatrix;\n"
                "uniform highp mat4 modelMatrix;\n"
                "uniform highp mat4 projectionMatrix;\n"

                "varying highp vec3 pixelPosition;\n"
                "varying highp vec3 pixelNormal;\n"
                "varying highp vec4 pixelColor;\n"
                "varying lowp float pixelSelection;\n"

                "void main(void)\n"
                "{\n"
                "    pixelPosition = position.xyz;\n"
                "    pixelNormal = normal;\n"
                "    pixelColor = color;\n"
                "    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);\n"
                "    pixelSelection = isSelected == 0u ? 0.0 : 1.0;\n"
                "}\n"
                ,

                // 3.14159265358979323846264338327950288
                "#version 130\n"

                "uniform sampler2D tex;\n"

                "uniform lowp float bwColor;\n"
                "uniform lowp float bwTexColor;\n"
                "uniform highp vec3 panoramaCenter;\n"

                "varying highp vec3 pixelPosition;\n"
                "varying highp vec3 pixelNormal;\n"
                "varying highp vec4 pixelColor;\n"
                "varying lowp float pixelSelection;\n"

                "void main(void)\n"
                "{\n"
                "    highp vec3 direction = pixelPosition - panoramaCenter;\n"
                "    highp float longi = atan(direction.y, direction.x);\n"
                "    highp float lati = asin(direction.z / length(direction));\n"
                "    highp vec2 texCoord = vec2(longi / 3.1415926535897932 / 2.0 + 0.5, - lati / 3.1415926535897932 + 0.5);\n"
                "    lowp vec4 texColor = texture2D(tex, texCoord);\n"
                "    gl_FragColor = (pixelColor * bwColor + texColor * bwTexColor)"
                "       / (bwColor + bwTexColor);\n"
                "    gl_FragColor.a = 1.0 - pixelSelection * 0.5;\n"
                "}\n"
            };



            switch (name) {
            case OpenGLShaderSourceDescriptor::DefaultPoints: return defaultPointsShaderSource;
            case OpenGLShaderSourceDescriptor::DefaultLines: return defaultLinesShaderSource;
            case OpenGLShaderSourceDescriptor::DefaultTriangles: return defaultTrianglesShaderSource;
            case OpenGLShaderSourceDescriptor::Panorama: return panoramaShaderSource;

            case OpenGLShaderSourceDescriptor::XPoints: return xPointsShaderSource;
            case OpenGLShaderSourceDescriptor::XLines: return xLinesShaderSource;
            case OpenGLShaderSourceDescriptor::XTriangles: return xTrianglesShaderSource;
            case OpenGLShaderSourceDescriptor::XPanorama: return xPanoramaShaderSource;
            default:
                return defaultTrianglesShaderSource;
            }
        }






      


    }

    namespace core {

        Box3 BoundingBox(const gui::SpatialProjectedPolygon & spp){
            std::vector<Vec3> cs(spp.corners.size());
            for (int i = 0; i < spp.corners.size(); i++){
                cs[i] = IntersectionOfLineAndPlane(Ray3(spp.projectionCenter, spp.corners[i] - spp.projectionCenter), 
                    spp.plane).position;
            }
            return BoundingBoxOfContainer(cs);
        }

    }

}