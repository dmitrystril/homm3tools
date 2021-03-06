// Created by John �kerblom 2015-08-03

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gzip_utils.h>

#ifdef _MSC_VER
    #pragma warning(disable:4996) // fopen unsafe
#endif

static const uint32_t crc32_table[] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

static const uint32_t crc32_reverse[] = {
    0x00000000, 0xDB710641, 0x6D930AC3, 0xB6E20C82, 0xDB261586, 0x005713C7,
    0xB6B51F45, 0x6DC41904, 0x6D3D2D4D, 0xB64C2B0C, 0x00AE278E, 0xDBDF21CF,
    0xB61B38CB, 0x6D6A3E8A, 0xDB883208, 0x00F93449, 0xDA7A5A9A, 0x010B5CDB,
    0xB7E95059, 0x6C985618, 0x015C4F1C, 0xDA2D495D, 0x6CCF45DF, 0xB7BE439E,
    0xB74777D7, 0x6C367196, 0xDAD47D14, 0x01A57B55, 0x6C616251, 0xB7106410,
    0x01F26892, 0xDA836ED3, 0x6F85B375, 0xB4F4B534, 0x0216B9B6, 0xD967BFF7,
    0xB4A3A6F3, 0x6FD2A0B2, 0xD930AC30, 0x0241AA71, 0x02B89E38, 0xD9C99879,
    0x6F2B94FB, 0xB45A92BA, 0xD99E8BBE, 0x02EF8DFF, 0xB40D817D, 0x6F7C873C,
    0xB5FFE9EF, 0x6E8EEFAE, 0xD86CE32C, 0x031DE56D, 0x6ED9FC69, 0xB5A8FA28,
    0x034AF6AA, 0xD83BF0EB, 0xD8C2C4A2, 0x03B3C2E3, 0xB551CE61, 0x6E20C820,
    0x03E4D124, 0xD895D765, 0x6E77DBE7, 0xB506DDA6, 0xDF0B66EA, 0x047A60AB,
    0xB2986C29, 0x69E96A68, 0x042D736C, 0xDF5C752D, 0x69BE79AF, 0xB2CF7FEE,
    0xB2364BA7, 0x69474DE6, 0xDFA54164, 0x04D44725, 0x69105E21, 0xB2615860,
    0x048354E2, 0xDFF252A3, 0x05713C70, 0xDE003A31, 0x68E236B3, 0xB39330F2,
    0xDE5729F6, 0x05262FB7, 0xB3C42335, 0x68B52574, 0x684C113D, 0xB33D177C,
    0x05DF1BFE, 0xDEAE1DBF, 0xB36A04BB, 0x681B02FA, 0xDEF90E78, 0x05880839,
    0xB08ED59F, 0x6BFFD3DE, 0xDD1DDF5C, 0x066CD91D, 0x6BA8C019, 0xB0D9C658,
    0x063BCADA, 0xDD4ACC9B, 0xDDB3F8D2, 0x06C2FE93, 0xB020F211, 0x6B51F450,
    0x0695ED54, 0xDDE4EB15, 0x6B06E797, 0xB077E1D6, 0x6AF48F05, 0xB1858944,
    0x076785C6, 0xDC168387, 0xB1D29A83, 0x6AA39CC2, 0xDC419040, 0x07309601,
    0x07C9A248, 0xDCB8A409, 0x6A5AA88B, 0xB12BAECA, 0xDCEFB7CE, 0x079EB18F,
    0xB17CBD0D, 0x6A0DBB4C, 0x6567CB95, 0xBE16CDD4, 0x08F4C156, 0xD385C717,
    0xBE41DE13, 0x6530D852, 0xD3D2D4D0, 0x08A3D291, 0x085AE6D8, 0xD32BE099,
    0x65C9EC1B, 0xBEB8EA5A, 0xD37CF35E, 0x080DF51F, 0xBEEFF99D, 0x659EFFDC,
    0xBF1D910F, 0x646C974E, 0xD28E9BCC, 0x09FF9D8D, 0x643B8489, 0xBF4A82C8,
    0x09A88E4A, 0xD2D9880B, 0xD220BC42, 0x0951BA03, 0xBFB3B681, 0x64C2B0C0,
    0x0906A9C4, 0xD277AF85, 0x6495A307, 0xBFE4A546, 0x0AE278E0, 0xD1937EA1,
    0x67717223, 0xBC007462, 0xD1C46D66, 0x0AB56B27, 0xBC5767A5, 0x672661E4,
    0x67DF55AD, 0xBCAE53EC, 0x0A4C5F6E, 0xD13D592F, 0xBCF9402B, 0x6788466A,
    0xD16A4AE8, 0x0A1B4CA9, 0xD098227A, 0x0BE9243B, 0xBD0B28B9, 0x667A2EF8,
    0x0BBE37FC, 0xD0CF31BD, 0x662D3D3F, 0xBD5C3B7E, 0xBDA50F37, 0x66D40976,
    0xD03605F4, 0x0B4703B5, 0x66831AB1, 0xBDF21CF0, 0x0B101072, 0xD0611633,
    0xBA6CAD7F, 0x611DAB3E, 0xD7FFA7BC, 0x0C8EA1FD, 0x614AB8F9, 0xBA3BBEB8,
    0x0CD9B23A, 0xD7A8B47B, 0xD7518032, 0x0C208673, 0xBAC28AF1, 0x61B38CB0,
    0x0C7795B4, 0xD70693F5, 0x61E49F77, 0xBA959936, 0x6016F7E5, 0xBB67F1A4,
    0x0D85FD26, 0xD6F4FB67, 0xBB30E263, 0x6041E422, 0xD6A3E8A0, 0x0DD2EEE1,
    0x0D2BDAA8, 0xD65ADCE9, 0x60B8D06B, 0xBBC9D62A, 0xD60DCF2E, 0x0D7CC96F,
    0xBB9EC5ED, 0x60EFC3AC, 0xD5E91E0A, 0x0E98184B, 0xB87A14C9, 0x630B1288,
    0x0ECF0B8C, 0xD5BE0DCD, 0x635C014F, 0xB82D070E, 0xB8D43347, 0x63A53506,
    0xD5473984, 0x0E363FC5, 0x63F226C1, 0xB8832080, 0x0E612C02, 0xD5102A43,
    0x0F934490, 0xD4E242D1, 0x62004E53, 0xB9714812, 0xD4B55116, 0x0FC45757,
    0xB9265BD5, 0x62575D94, 0x62AE69DD, 0xB9DF6F9C, 0x0F3D631E, 0xD44C655F,
    0xB9887C5B, 0x62F97A1A, 0xD41B7698, 0x0F6A70D9
};

// From python code: http://blog.stalkr.net/2011/03/crc-32-forging.html
static uint32_t _crc32_forge_end(uint32_t wanted_crc, uint8_t *data, size_t n)
{
    uint32_t fwd_crc;
    uint32_t bkd_crc;
    uint8_t *p;

    // forward calculation of CRC up to pos, sets current forward CRC state
    fwd_crc = 0xFFFFFFFF;
    for (unsigned int i = 0; i < n; ++i) {
        fwd_crc = (fwd_crc >> 8) ^ crc32_table[(fwd_crc ^ data[i]) & 0xFF];
    }

    // backward calculation of CRC up to pos, sets wanted backward CRC state
    bkd_crc = wanted_crc ^ 0xFFFFFFFF;

    // deduce the 4 bytes we need to insert
    p = ((uint8_t *)&fwd_crc) + 3;
    for (int i = 0; i < sizeof(uint32_t); ++i) {
        bkd_crc = ((bkd_crc << 8) & 0xFFFFFFFF) ^ crc32_reverse[bkd_crc >> 24] ^ *(p--);
    }

    return bkd_crc;
}

void _print_usage(void)
{
    puts("h3mdemosign <input .h3m map> [output .h3m map=h3demo.h3m]");
    puts("Examples:");
    puts("\th3mdemosign Arrogance.h3m Arrogance_demo.h3m");
    puts("\th3mdemosign Arrogance.h3m");
}

int main(int argc, char **argv)
{
    FILE *f_out = NULL;
    uint8_t *buf_in = NULL;
    uint8_t *buf_out = NULL;
    long size_in = 0;
    uint32_t forged_end;
    size_t written;
    const char *output_file;

    if (argc != 2 && argc != 3) {
        _print_usage();
        return 1;
    }

    // Open output map for writing
    output_file = (argc == 3) ? argv[2] : "h3demo.h3m";
    f_out = fopen(output_file, "wb");
    if (f_out == NULL) {
        printf("[!] Failed to open output file %s for writing!", output_file);
        return 1;
    }

    // Decompress input map into buffer. If not compressed, contents are copied
    if (0 != gu_decompress_file_to_mem(argv[1], &buf_in, &size_in)) {
        printf("[!] Failed to decompress/read input .h3m map %s!", argv[1]);
        fclose(f_out);
        return 1;
    }

    // Calculate forged end of file in order to match checksum 0xFEEFB9EB -
    // the checksum of h3demo.h3m that comes with the Heroes III demo.
    forged_end = _crc32_forge_end(0xFEEFB9EB, buf_in, size_in);

    // Allocate 4 extra bytes for the output file to hold the forged end
    buf_out = malloc(size_in + 4);
    if (buf_out == NULL) {
        puts("[!] Failed to allocate memory!");
        gu_free(buf_in);
        return 1;
    }

    // Copy original file and forged end into output buffer
    memcpy(buf_out, buf_in, size_in);
    *(uint32_t *)&buf_out[size_in] = forged_end;

    // Write map with forged end
    if (size_in + 4 != (written = fwrite(buf_out, 1, size_in + 4, f_out))) {
        printf("[!] Could only write %d/%d bytes!", written, size_in + 4);
        gu_free(buf_in);
        fclose(f_out);
        return 1;
    }

    printf("[+] Signed %s as %s", argv[1], output_file);

    // Cleanup
    gu_free(buf_in);
    fclose(f_out);

    return 0;
}

