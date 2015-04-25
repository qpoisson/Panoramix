#ifdef USE_MATLAB
#include <mex.h>
#include <engine.h>
#endif

#include "matlab.hpp"

namespace panoramix {
    namespace misc {

        using namespace core;

#ifdef USE_MATLAB
        namespace {

            // template helpers
            template <typename T>	struct RMXType	{ static const mxClassID ClassID = mxUNKNOWN_CLASS; };
            template <int ID>		struct RMXID	{ using type = void; };

#define CONNECT_CLASSID(T, id) \
    template <> struct RMXType<T>	{ static const mxClassID ClassID = id; }; \
    template <> struct RMXID<id>	{ using type = T; }

            CONNECT_CLASSID(mxLogical, mxLOGICAL_CLASS);
            CONNECT_CLASSID(double, mxDOUBLE_CLASS);
            CONNECT_CLASSID(float, mxSINGLE_CLASS);
            CONNECT_CLASSID(int8_t, mxINT8_CLASS);
            CONNECT_CLASSID(int16_t, mxINT16_CLASS);
            CONNECT_CLASSID(int32_t, mxINT32_CLASS);
            CONNECT_CLASSID(int64_t, mxINT64_CLASS);
            CONNECT_CLASSID(uint8_t, mxUINT8_CLASS);
            CONNECT_CLASSID(uint16_t, mxUINT16_CLASS);
            CONNECT_CLASSID(uint32_t, mxUINT32_CLASS);
            CONNECT_CLASSID(uint64_t, mxUINT64_CLASS);

            template <typename T>
            inline mxArray* CreateNumericMatrix(const T* d, int m, int n) {
                mxArray* ma = mxCreateNumericMatrix(m, n, RMXType<T>::ClassID, mxREAL);
                std::memcpy(mxGetData(ma), d, sizeof(T)*m*n);
                return ma;
            }
            template <typename T>
            inline mxArray* CreateNumericMatrix(int m, int n){
                return mxCreateNumericMatrix(m, n, RMXType<T>::ClassID, mxREAL);
            }

            inline mxClassID CVDepthToMxClassID(int cvDepth){
                switch (cvDepth){
                case CV_8U: return mxUINT8_CLASS;
                case CV_8S: return mxINT8_CLASS;
                case CV_16U: return mxUINT16_CLASS;
                case CV_16S: return mxINT16_CLASS;
                case CV_32S: return mxINT32_CLASS;
                case CV_32F: return mxSINGLE_CLASS;
                case CV_64F: return mxDOUBLE_CLASS;
                default:
                    std::cout << "this cv depth type cannot be converted to a matlab class id" << std::endl;
                    return mxUNKNOWN_CLASS;
                }
            }

            inline int MxClassIDToCVDepth(mxClassID id){
                switch (id){
                case mxLOGICAL_CLASS: return CV_8U;
                case mxDOUBLE_CLASS: return CV_64F;
                case mxSINGLE_CLASS: return CV_32F;
                case mxINT8_CLASS: return CV_8S;
                case mxINT16_CLASS: return CV_16S;
                case mxINT32_CLASS: return CV_32S;
                case mxUINT8_CLASS: return CV_8U;
                case mxUINT16_CLASS: return CV_16U;
                case mxUINT32_CLASS: return CV_32S;
                default:
                    std::cout << "this matlab class id cannot be converted to a cv depth type" << std::endl;
                    return -1;
                }
            }
        }


        class MXArray;

        class MXArray {
        public:
            MXArray() : _mxa(nullptr), _destroyWhenOutofScope(false) {}
            MXArray(mxArray * mxa, bool dos = false) : _mxa(mxa), _destroyWhenOutofScope(dos){}
            MXArray(MXArray && a) {
                _mxa = a._mxa;
                a._mxa = nullptr;
                _destroyWhenOutofScope = a._destroyWhenOutofScope;
                a._destroyWhenOutofScope = false;
            }
            virtual ~MXArray() {
                if (_destroyWhenOutofScope){
                    mxDestroyArray(_mxa);
                }
                _mxa = nullptr;
            }

        public:
            mxClassID classID() const { return mxGetClassID(_mxa); }
            void * data() const { return mxGetData(_mxa); }

#define DECL_MXARRAY_MEMBERFUNCTION_IS(what) bool is##what() const { return mxIs##what(_mxa); }

            DECL_MXARRAY_MEMBERFUNCTION_IS(Numeric)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Cell)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Logical)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Char)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Struct)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Opaque)
            DECL_MXARRAY_MEMBERFUNCTION_IS(FunctionHandle)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Object)

            DECL_MXARRAY_MEMBERFUNCTION_IS(Complex)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Sparse)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Double)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Single)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Int8)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Uint8)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Int16)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Uint16)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Int32)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Uint32)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Int64)
            DECL_MXARRAY_MEMBERFUNCTION_IS(Uint64)


            template <class T> bool is() const { return false; }
            template <> bool is<double>() const { return isDouble(); }
            template <> bool is<float>() const { return isSingle(); }
            template <> bool is<int8_t>() const { return isInt8(); }
            template <> bool is<uint8_t>() const { return isUint8(); }
            template <> bool is<int16_t>() const { return isInt16(); }
            template <> bool is<uint16_t>() const { return isUint16(); }
            template <> bool is<int32_t>() const { return isInt32(); }
            template <> bool is<uint32_t>() const { return isUint32(); }
            template <> bool is<int64_t>() const { return isInt64(); }
            template <> bool is<uint64_t>() const { return isUint64(); }

            size_t m() const { return mxGetM(_mxa); }
            size_t n() const { return mxGetN(_mxa); }

            bool empty() const { return mxIsEmpty(_mxa); }

            bool isFromGlobalWorkspace() const { return mxIsFromGlobalWS(_mxa); }

            double scalar() const { return mxGetScalar(_mxa); }
            std::string toString() const {
                size_t len = nelements();
                std::string str(len, '\0');
                mxGetString(_mxa, &str[0], len);
                return str;
            }

            operator double() const { return scalar(); }
            operator std::string() const { return toString(); }

            size_t nelements() const { return mxGetNumberOfElements(_mxa); }
            mwSize nzmax() const { return mxGetNzmax(_mxa); }

            MXArray cell(mwIndex i) const { return mxGetCell(_mxa, i); }

            int nfields() const { return mxGetNumberOfFields(_mxa); }
            const char * fieldName(int n) const { return mxGetFieldNameByNumber(_mxa, n); }
            int fieldNumber(const std::string & name) const { return mxGetFieldNumber(_mxa, name.c_str()); }
            MXArray field(const std::string & name, int i) const { return mxGetField(_mxa, i, name.c_str()); }

            MXArray property(const std::string & name, mwIndex i) const { return mxGetProperty(_mxa, i, name.c_str()); }

            const char * className() const { return mxGetClassName(_mxa); }

            mwSize ndims() const { return mxGetNumberOfDimensions(_mxa); }
            std::vector<mwSize> dims() const {
                std::vector<mwSize> ds(ndims());
                std::copy_n(mxGetDimensions(_mxa), ds.size(), ds.begin());
                return ds;
            }
            mwSize dim(int d) const {
                return dims().at(d);
            }

            template <class ... Ints>
            mwIndex offset(Ints... subs) const {
                mwIndex sbs[sizeof...(Ints)] = { subs ... };
                return mxCalcSingleSubscript(_mxa, sizeof...(Ints), sbs);
            }

            template <class T, class ... Ints, class = std::enable_if_t<std::is_arithmetic<T>::value>>
            const T & at(Ints... subs) const {
                return static_cast<const T*>(mxGetData(_mxa))[offset(subs...)];
            }
            template <class T, class ... Ints, class = std::enable_if_t<std::is_arithmetic<T>::value>>
            T & at(Ints... subs) {
                return static_cast<T*>(mxGetData(_mxa))[offset(subs...)];
            }

            void setData(void* d) { mxSetData(_mxa, d); }

            void setM(size_t m) { mxSetM(_mxa, m); }
            void setN(size_t n) { mxSetN(_mxa, n); }

            void setIsFromGlobalWorkspace(bool b) { mxSetFromGlobalWS(_mxa, b); }

            void setCell(mwIndex i, const MXArray & a) { mxSetCell(_mxa, i, a._mxa); }

            void setField(const std::string & name, int i, const MXArray & a) { mxSetField(_mxa, i, name.c_str(), a._mxa); }
            void setProperty(const std::string & name, int i, const MXArray & a) { mxSetProperty(_mxa, i, name.c_str(), a._mxa); }


        protected:
            mxArray * _mxa;
            bool _destroyWhenOutofScope;
        };





        class MatlabEngine {
        public:
            inline explicit MatlabEngine(const char * cmd = nullptr, int bufferSize = 5e4)
                : _engine(nullptr), _buffer(nullptr) {
                _engine = engOpen(cmd);
                if (_engine){
                    engSetVisible(_engine, false);
                    _buffer = new char[bufferSize];
                    std::memset(_buffer, 0, bufferSize);
                    engOutputBuffer(_engine, _buffer, bufferSize);
                    std::cout << "Matlab Engine Launched" << std::endl;
                    engEvalString(_engine, ("cd " + Matlab::DefaultCodeDir() + "; startup; pwd").c_str());
                    std::cout << _buffer << std::endl;
                }
            }
            inline ~MatlabEngine() {
                if (_engine){
                    engClose(_engine);
                    std::cout << "Matlab Engine Closed" << std::endl;
                }
                delete[] _buffer;
            }
            inline engine * eng() const { return _engine; }
            inline char * buffer() const { return _buffer; }
        private:
            engine * _engine;
            char * _buffer;
        };

        static MatlabEngine * _Engine = nullptr;
        static int _EngineRefCount = 0;


        bool Matlab::IsBuilt() { return true; }

        bool Matlab::StartEngine(){
            _EngineRefCount++;
            if (_EngineRefCount > 0 && !_Engine){
                _Engine = new MatlabEngine;
                if (_Engine->eng() == nullptr){
                    delete _Engine;
                    _Engine = nullptr;
                }
            }
            std::cout << "Matlab Engine RefCount: " << _EngineRefCount << std::endl;
            return _Engine != nullptr;
        }

        void Matlab::CloseEngine(){
            if (_EngineRefCount <= 0){
                _EngineRefCount = 0;
                return;
            }
            _EngineRefCount--;
            std::cout << "Matlab Engine RefCount: " << _EngineRefCount << std::endl;
            if (_EngineRefCount <= 0){
                _EngineRefCount = 0;
                if (_Engine){
                    delete _Engine;
                    _Engine = nullptr;
                }
            }
        }

        bool Matlab::EngineStarted(){
            return _Engine && _Engine->eng();
        }

        

        std::string Matlab::DefaultCodeDir() { return MATLAB_CODE_DIR; }



        bool Matlab::RunScript(const char * cmd) {
            if (!_Engine)
                return false;
            bool ret = engEvalString(_Engine->eng(), cmd) == 0;
            if (strlen(_Engine->buffer()) > 0){
                std::cout << "[Message when executing '" << cmd << "']:\n" << _Engine->buffer() << std::endl;
            }
            return ret;
        }


        const char * Matlab::LastMessage(){
            return _Engine->buffer();
        }

        void * Matlab::PutVariable(CVInputArray mat){            
            cv::Mat& im = mat.getMat();

            // collect all dimensions of im
            int channelNum = im.channels();
            mwSize * dimSizes = new mwSize[im.dims + 1];
            for (int i = 0; i < im.dims; i++)
                dimSizes[i] = im.size[i];
            dimSizes[im.dims] = channelNum;

            // create mxArray
            mxArray* ma = mxCreateNumericArray(im.dims + 1, dimSizes, CVDepthToMxClassID(im.depth()), mxREAL);
            delete[] dimSizes;

            if (!ma)
                return false;

            uint8_t * mad = (uint8_t*)mxGetData(ma);
            const size_t szForEachElem = im.elemSize1();
            mwIndex * mxIndices = new mwIndex[im.dims + 1];
            int * cvIndices = new int[im.dims];

            cv::MatConstIterator iter(&im);
            int imTotal = im.total();
            for (int i = 0; i < imTotal; i++, ++iter){
                // get indices in cv::Mat
                iter.pos(cvIndices);
                // copy indices to mxIndices
                std::copy(cvIndices, cvIndices + im.dims, mxIndices);
                for (mwIndex k = 0; k < channelNum; k++){
                    const uint8_t * fromDataHead = (*iter) + k * szForEachElem;
                    mxIndices[im.dims] = k; // set the last indices
                    uint8_t * toDataHead = mad + mxCalcSingleSubscript(ma, im.dims + 1, mxIndices) * szForEachElem;
                    std::memcpy(toDataHead, fromDataHead, szForEachElem);
                }
            }

            delete[] mxIndices;
            delete[] cvIndices;

            return ma;
        }

        bool Matlab::PutVariable(const char * name, CVInputArray mat){
            if (!_Engine)
                return false;
            
            mxArray * ma = static_cast<mxArray*>(PutVariable(mat));
            if (!ma)
                return false;

            int result = engPutVariable(_Engine->eng(), name, ma);
            mxDestroyArray(ma);
            return result == 0;
        }


        bool Matlab::GetVariable(const void * mxData, CVOutputArray mat, bool lastDimIsChannel){
            if (!mat.needed())
                return true;
            if (!mxData)
                return false;
            const mxArray * ma = static_cast<const mxArray*>(mxData);
            int d = mxGetNumberOfDimensions(ma);
            assert((d >= 2) && "dimension num of the variable must be over 2");
            const mwSize * dimSizes = mxGetDimensions(ma);

            // set channels, dims and dimSizes
            int channels = 0;
            int cvDims = 0;
            int * cvDimSizes = nullptr;

            if (lastDimIsChannel && d > 2){
                channels = dimSizes[d - 1];
                cvDims = d - 1;
                cvDimSizes = new int[d - 1];
                std::copy(dimSizes, dimSizes + d - 1, cvDimSizes);
            }
            else{
                channels = 1;
                cvDims = d;
                cvDimSizes = new int[d];
                std::copy(dimSizes, dimSizes + d, cvDimSizes);
            }

            const size_t szForEachElem = mxGetElementSize(ma);
            int depth = MxClassIDToCVDepth(mxGetClassID(ma));
            if (depth == -1){
                delete[] cvDimSizes;
                return false;
            }

            const uint8_t * mad = (const uint8_t*)mxGetData(ma);
            // create Mat
            mat.create(cvDims, cvDimSizes, CV_MAKETYPE(depth, channels));
            cv::Mat im = mat.getMat();

            mwIndex * mxIndices = new mwIndex[im.dims + 1];
            int * cvIndices = new int[im.dims];

            cv::MatConstIterator iter(&im);
            int imTotal = im.total();
            for (int i = 0; i < imTotal; i++, ++iter){
                // get indices in cv::Mat
                iter.pos(cvIndices);
                // copy indices to mxIndices
                std::copy(cvIndices, cvIndices + im.dims, mxIndices);
                for (mwIndex k = 0; k < channels; k++){
                    uint8_t * toDataHead = (*iter) + k * szForEachElem;
                    mxIndices[im.dims] = k; // set the last indices
                    const uint8_t * fromDataHead = mad + mxCalcSingleSubscript(ma, im.dims + 1, mxIndices) * szForEachElem;
                    std::memcpy(toDataHead, fromDataHead, szForEachElem);
                }
            }

            delete[] cvDimSizes;
            delete[] mxIndices;
            delete[] cvIndices;
            return true;
        }

        bool Matlab::GetVariable(const char * name, CVOutputArray mat, bool lastDimIsChannel){
            if (!_Engine)
                return false;

            if (!mat.needed())
                return true;
            
            mxArray * ma = engGetVariable(_Engine->eng(), name);
            if (!ma)
                return false;

            bool result = GetVariable(ma, mat, lastDimIsChannel);
            mxDestroyArray(ma);
            return result;
        }

        bool Matlab::GetVariable(const char * name, double & a){
            if (!_Engine)
                return false;

            mxArray * ma = engGetVariable(_Engine->eng(), name);
            if (!ma)
                return false;

            bool result = GetVariable((const void*)ma, a);
            mxDestroyArray(ma);
            return result;
        }

        bool Matlab::GetVariable(const void * mxData, double & a){
            if (!mxData)
                return false;
            const mxArray * ma = static_cast<const mxArray*>(mxData);
            if (!ma)
                return false;
            a = mxGetScalar(ma);
            return true;
        }


        bool Matlab::GetVariable(const char * name, std::string & a){
            if (!_Engine)
                return false;

            mxArray * ma = engGetVariable(_Engine->eng(), name);
            if (!ma)
                return false;

            bool result = GetVariable((const void*)ma, a);
            mxDestroyArray(ma);
            return result;
        }

        bool Matlab::GetVariable(const void * mxData, std::string  & a){
            if (!mxData)
                return false;
            const mxArray * ma = static_cast<const mxArray*>(mxData);
            if (!ma)
                return false;
            char buff[1024];
            std::fill_n(buff, 1024, '\0');
            mxGetString(ma, buff, 1024);
            a = buff;
            return true;
        }



        void * Matlab::PutVariable(const cv::SparseMat & mat){

            auto & im = mat;

            // collect all dimensions of im
            int channelNum = im.channels();
            assert(channelNum == 1);

            // create mxArray
            int nzc = mat.nzcount();
            mxArray* ma = mxCreateSparse(im.size(0), im.size(1), nzc, mxREAL);

            if (!ma)
                return nullptr;

            std::vector<std::tuple<int, int, double>> triplets; // col - row - val
            triplets.reserve(nzc);

            THERE_ARE_BOTTLENECKS_HERE(); // we should only iterate non zero elements
            for (auto iter = mat.begin(); iter != mat.end(); ++iter){
                int ii = iter.node()->idx[0];
                int jj = iter.node()->idx[1];
                uchar* data = iter.ptr;
                double v = 0.0;
                if (mat.type() == CV_32FC1){
                    float value = 0.0f;
                    std::memcpy(&value, data, sizeof(value));
                    v = value;
                }
                else if (mat.type() == CV_64FC1){
                    double value = 0.0f;
                    std::memcpy(&value, data, sizeof(value));
                    v = value;
                }
                else if (mat.type() == CV_32SC1){
                    int32_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    v = value;
                }
                else if (mat.type() == CV_64FC1){
                    int64_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    v = value;
                }
                else if (mat.type() == CV_8UC1){
                    uint8_t value = 0;
                    std::memcpy(&value, data, sizeof(value));
                    v = value;
                }
                else{
                    assert(false && "element type is not supported here!");
                }
                if (v != 0.0)
                    triplets.emplace_back(jj, ii, v); // col - row - val
            }

            // make triplets ordered in column indices to satisfy matlab interface
            std::sort(triplets.begin(), triplets.end());

            // fill in matlab data
            auto sr = mxGetPr(ma);
            //auto si = mxGetPi(ma);
            auto irs = mxGetIr(ma);
            auto jcs = mxGetJc(ma);
            jcs = (mwIndex*)mxRealloc(jcs, (im.size(1) + 1) * sizeof(mwIndex));
            std::fill(jcs, jcs + (im.size(1) + 1), 0);
            mxSetJc(ma, jcs);

            for (int i = 0; i < triplets.size(); i++){
                sr[i] = std::get<2>(triplets[i]);
                int ii = std::get<1>(triplets[i]) + 1;
                int jj = std::get<0>(triplets[i]) + 1;
                irs[i] = ii - 1;
                jcs[jj] ++;
            }

            for (int j = 1; j < (im.size(1) + 1); j++){
                jcs[j] += jcs[j - 1];
            }

            return ma;

        }

        bool Matlab::PutVariable(const char * name, const cv::SparseMat & mat){
            if (!_Engine)
                return false;
            
            mxArray * ma = static_cast<mxArray*>(PutVariable(mat));
            if (!ma)
                return false;

            int result = engPutVariable(_Engine->eng(), name, ma);
            
            mxDestroyArray(ma);
            return result == 0;
        }


#else
        bool Matlab::IsBuilt() {return false;}
        bool Matlab::StartEngine() {return false;}
        void Matlab::CloseEngine() {}
        bool Matlab::EngineStarted(){ return false;}
        std::string Matlab::DefaultCodeDir() { return ""; }
        bool Matlab::IsUsable() {return false;}
        bool Matlab::RunScript(const char *) {return false;}
        const char * Matlab::LastMessage() {return nullptr;}
        bool Matlab::PutVariable(const char * name, CVInputArray a) {return false;}
        bool Matlab::GetVariable(const char * name, CVOutputArray a, bool lastDimIsChannel) {return false;}
        bool Matlab::PutVariable(const char * name, const cv::SparseMat & mat) { return false; }
#endif

    }
}