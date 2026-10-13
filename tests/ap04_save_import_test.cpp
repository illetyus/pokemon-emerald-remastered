#include "RemasterSaveImport.h"
#include "remaster/platform.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
namespace fs = std::filesystem;
static unsigned checks, failures;
static void check(bool value, const char* message) {
    ++checks; if (!value) { ++failures; std::cerr << "save import: " << message << '\n'; }
}
static std::vector<uint8_t> read(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
static bool write(const fs::path& p, const std::vector<uint8_t>& data) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    f.close(); return !f.fail();
}
static void put16(uint8_t* p, unsigned value) { p[0]=static_cast<uint8_t>(value); p[1]=static_cast<uint8_t>(value>>8); }
static void put32(uint8_t* p, uint32_t value) { put16(p,value); put16(p+2,value>>16); }
static std::vector<uint8_t> fixture(bool stock, bool future = false) {
    // R17 codec fixture only. This is NOT a canonical NewGame oracle.
    std::vector<uint8_t> image(131072, 0xED);
    for (unsigned slot=0;slot<2;++slot) for(unsigned id=0;id<14;++id) {
        auto* p=image.data()+(slot*14+(id+3)%14)*4096;
        std::memset(p,0,3968);
        const size_t span=id==0 ? (stock ? 0xF2C : 0xF44)
            : id==4 ? (stock ? 0xF08 : 0xF48) : id==13 ? 0x7D0 : 3968;
        if (id==0) { p[8]=1; p[0]=0xBF; p[1]=0xFF; }
        if (id==1) { put16(p,17); put16(p+2,21); }
        if (future && !stock && id==4) { put32(p+0x758,0x35504956); p[0x75C]=99; }
        uint32_t sum=0;
        for(size_t i=0;i<span;i+=4) sum+=static_cast<uint32_t>(p[i])|(static_cast<uint32_t>(p[i+1])<<8)
            |(static_cast<uint32_t>(p[i+2])<<16)|(static_cast<uint32_t>(p[i+3])<<24);
        put16(p+4084,id); put16(p+4086,(sum>>16)+(sum&65535));
        put32(p+4088,0x08012025); put32(p+4092,slot ? 41 : 40);
    }
    return image;
}
static std::vector<uint8_t> platformImage;
static unsigned platformWrites;
static RemasterSaveReadResult memoryRead(void*,const char*,uint8_t* buffer,size_t capacity,size_t* size) {
    *size=platformImage.size();
    if (*size<=capacity) std::memcpy(buffer,platformImage.data(),*size);
    return REMASTER_SAVE_READ_OK;
}
static int memoryWrite(void*,const char*,const uint8_t* buffer,size_t size) {
    ++platformWrites; platformImage.assign(buffer,buffer+size); return 1;
}
static std::vector<uint8_t> permuted(std::vector<uint8_t> image) {
    for(unsigned slot=0;slot<2;++slot) {
        auto first=image.begin()+(slot*14+7)*4096; // logical section4, rotation3
        auto second=image.begin()+(slot*14+8)*4096; // logical section5, rotation3
        std::swap_ranges(first,first+4096,second); // keep complete headers/checksums intact
    }
    return image;
}
int main() {
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path dir=fs::temp_directory_path()/("ap04-import-"+std::to_string(stamp));
    check(fs::create_directory(dir),"private native temp directory");
    struct Cleanup { fs::path path; ~Cleanup(){ std::error_code e; fs::remove_all(path,e); } } cleanup{dir};
    const fs::path source=dir/fs::u8path("kay\xC4\xB1t-\xC3\xA7.sav"), copy=dir/"owned-copy.sav";
    RemasterEmeraldSave output{}, sentinel{}; std::memset(&sentinel,0xA7,sizeof sentinel); output=sentinel;
    const auto attempt=[&](const fs::path& src,const fs::path& dst,RemasterEmeraldSaveFormat format,
        RemasterEmeraldSaveStatus expected,bool copied) {
        output=sentinel;
        const auto result=RemasterSaveImport::CopyFirst(src,dst,format,output);
        check(result.status==expected,"typed result"); check(result.copyCreated==copied,"honest copy publication");
        if(expected!=REMASTER_EMERALD_SAVE_OK && expected!=REMASTER_EMERALD_SAVE_DEGRADED)
            check(!std::memcmp(&output,&sentinel,sizeof output),"failed import leaves prior usable state unchanged");
        return result;
    };
    for (bool stock : {false,true}) {
        const auto image=fixture(stock); check(write(source,image),"source fixture");
        const auto format=stock ? REMASTER_EMERALD_SAVE_FORMAT_STOCK : REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS;
        attempt(source,copy,format,REMASTER_EMERALD_SAVE_OK,true);
        check(read(source)==image && read(copy)==image,"all source/other-slot/special bytes copied unchanged");
        check(output.source_is_stock==static_cast<uint8_t>(stock) && output.counter==41 && output.selected_slot==1,"explicit format and selected counter retained");
        check(output.save_block1[0]==17 && output.save_block1[2]==21,"decode actual source domains");
        const auto prior=read(copy); attempt(source,copy,format,REMASTER_EMERALD_SAVE_IO_ERROR,false);
        check(read(copy)==prior && read(source)==image,"existing owned copy cannot be overwritten");
        attempt(source,source,format,REMASTER_EMERALD_SAVE_IO_ERROR,false);
        check(read(source)==image,"same-path source cannot be overwritten");
        check(fs::remove(copy),"remove only test-owned copy");
    }
    const auto valid=fixture(false); check(write(source,valid),"restore source fixture");
    for(auto format : {REMASTER_EMERALD_SAVE_FORMAT_UNSPECIFIED,static_cast<RemasterEmeraldSaveFormat>(99)}) {
        attempt(source,copy,format,REMASTER_EMERALD_SAVE_UNSUPPORTED,false);
        check(!fs::exists(copy) && read(source)==valid,"missing provenance never creates a copy or modifies source");
    }
    attempt(dir/"missing.sav",copy,REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,REMASTER_EMERALD_SAVE_EMPTY,false);
    attempt(dir,copy,REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,REMASTER_EMERALD_SAVE_IO_ERROR,false);
    attempt(source,dir/"absent-parent"/"copy.sav",REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,REMASTER_EMERALD_SAVE_IO_ERROR,false);
    for(const size_t n : {size_t(0),size_t(12),size_t(131073)}) {
        check(write(source,std::vector<uint8_t>(n,0xAB)),"wrong size fixture");
        attempt(source,copy,REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,REMASTER_EMERALD_SAVE_UNSUPPORTED,false);
        check(!fs::exists(copy),"unbounded or unsupported file is not copied");
    }
    auto corrupt=valid;
    for(unsigned slot=0;slot<2;++slot) corrupt[(slot*14+3)*4096+6]^=1;
    check(write(source,corrupt),"corrupt fixture");
    attempt(source,copy,REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,REMASTER_EMERALD_SAVE_CORRUPT,true);
    check(read(copy)==corrupt && read(source)==corrupt,"quarantined bad copy retained without touching original");
    check(fs::remove(copy),"remove bad test copy");
    auto future=fixture(false,true); check(write(source,future),"future metadata fixture");
    attempt(source,copy,REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,REMASTER_EMERALD_SAVE_UNSUPPORTED,true);
    check(read(copy)==future && read(source)==future,"future data quarantined unchanged");
    check(fs::remove(copy),"remove future test copy");
    const auto shuffledFuture=permuted(future);
    check(write(source,shuffledFuture),"checksum-valid noncanonical sector permutation fixture");
    attempt(source,copy,REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,REMASTER_EMERALD_SAVE_UNSUPPORTED,true);
    check(read(copy)==shuffledFuture && read(source)==shuffledFuture,"permuted future metadata quarantined unchanged");
    check(fs::remove(copy),"remove permuted future test copy");
    const auto shuffledValid=permuted(valid);
    check(write(source,shuffledValid),"supported noncanonical permutation fixture");
    attempt(source,copy,REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,REMASTER_EMERALD_SAVE_OK,true);
    check(read(copy)==shuffledValid && output.counter==41,"supported explicit-sector decoding remains valid");
    check(fs::remove(copy),"remove permuted valid test copy");
    RemasterPlatformVTable table{};table.save_read_result=memoryRead;table.save_write=memoryWrite;
    remaster_platform_install(&table);platformImage=shuffledFuture;platformWrites=0;
    std::vector<uint8_t> scratch(131072);
    RemasterEmeraldSave loaded{},stored{};
    check(remaster_emerald_save_load_platform_format("owned",scratch.data(),scratch.size(),
        REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,&loaded)==REMASTER_EMERALD_SAVE_UNSUPPORTED,
        "platform load checks actual logical section4 metadata");
    check(remaster_emerald_save_decode_format(valid.data(),valid.size(),
        REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,&stored)==REMASTER_EMERALD_SAVE_OK,"valid pending wrapper");
    const auto storedBefore=stored;
    check(!remaster_emerald_save_store_platform("owned",scratch.data(),scratch.size(),&stored),
        "store cannot replace unsupported permuted on-disk image");
    check(platformWrites==0 && platformImage==shuffledFuture && !std::memcmp(&stored,&storedBefore,sizeof stored),
        "future source rejection preserves file, counter and complete pending wrapper");
    remaster_platform_install(nullptr);
    auto degraded=valid; degraded[3*4096+6]^=1;
    check(write(source,degraded),"single-slot fallback fixture");
    attempt(source,copy,REMASTER_EMERALD_SAVE_FORMAT_VANILLAPLUS,REMASTER_EMERALD_SAVE_DEGRADED,true);
    check(output.counter==41 && read(copy)==degraded,"degraded exact copy can be imported explicitly");
    std::cout << "AP04 import " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
