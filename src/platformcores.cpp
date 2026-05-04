#include "platformcores.h"
#include <QDir>
#include <QStandardPaths>

const QHash<QString, QStringList> &platformCoreMap()
{
    static const QHash<QString, QStringList> map = {
        // Nintendo
        {QStringLiteral("nes"), {QStringLiteral("nestopia_libretro.so"), QStringLiteral("fceumm_libretro.so"), QStringLiteral("quicknes_libretro.so")}},
        {QStringLiteral("famicom"), {QStringLiteral("nestopia_libretro.so"), QStringLiteral("fceumm_libretro.so")}},
        {QStringLiteral("snes"), {QStringLiteral("snes9x_libretro.so"), QStringLiteral("bsnes_libretro.so"), QStringLiteral("snes9x2010_libretro.so")}},
        {QStringLiteral("sfc"), {QStringLiteral("snes9x_libretro.so"), QStringLiteral("bsnes_libretro.so")}},
        {QStringLiteral("n64"), {QStringLiteral("mupen64plus_next_libretro.so"), QStringLiteral("parallel_n64_libretro.so")}},
        {QStringLiteral("gb"), {QStringLiteral("gambatte_libretro.so"), QStringLiteral("sameboy_libretro.so"), QStringLiteral("gearboy_libretro.so")}},
        {QStringLiteral("gbc"), {QStringLiteral("gambatte_libretro.so"), QStringLiteral("sameboy_libretro.so"), QStringLiteral("gearboy_libretro.so")}},
        {QStringLiteral("gba"), {QStringLiteral("mgba_libretro.so"), QStringLiteral("vba_next_libretro.so")}},
        {QStringLiteral("nds"), {QStringLiteral("desmume_libretro.so"), QStringLiteral("melonds_libretro.so")}},
        {QStringLiteral("3ds"), {QStringLiteral("citra_libretro.so")}},
        {QStringLiteral("virtualboy"), {QStringLiteral("beetle_vb_libretro.so")}},
        {QStringLiteral("gamecube"), {QStringLiteral("dolphin_libretro.so")}},
        {QStringLiteral("wii"), {QStringLiteral("dolphin_libretro.so")}},
        {QStringLiteral("pokemini"), {QStringLiteral("pokemini_libretro.so")}},

        // Sony
        {QStringLiteral("ps"), {QStringLiteral("pcsx_rearmed_libretro.so"), QStringLiteral("beetle_psx_hw_libretro.so")}},
        {QStringLiteral("psx"), {QStringLiteral("pcsx_rearmed_libretro.so"), QStringLiteral("beetle_psx_hw_libretro.so")}},
        {QStringLiteral("ps2"), {QStringLiteral("pcsx2_libretro.so")}},
        {QStringLiteral("psp"), {QStringLiteral("ppsspp_libretro.so")}},

        // Sega
        {QStringLiteral("megadrive"), {QStringLiteral("genesis_plus_gx_libretro.so"), QStringLiteral("picodrive_libretro.so")}},
        {QStringLiteral("genesis"), {QStringLiteral("genesis_plus_gx_libretro.so"), QStringLiteral("picodrive_libretro.so")}},
        {QStringLiteral("mastersystem"),
         {QStringLiteral("genesis_plus_gx_libretro.so"), QStringLiteral("picodrive_libretro.so"), QStringLiteral("smsplus_libretro.so")}},
        {QStringLiteral("sms"),
         {QStringLiteral("genesis_plus_gx_libretro.so"), QStringLiteral("picodrive_libretro.so"), QStringLiteral("smsplus_libretro.so")}},
        {QStringLiteral("gamegear"), {QStringLiteral("genesis_plus_gx_libretro.so"), QStringLiteral("smsplus_libretro.so")}},
        {QStringLiteral("sega32x"), {QStringLiteral("picodrive_libretro.so")}},
        {QStringLiteral("sega32"), {QStringLiteral("picodrive_libretro.so")}},
        {QStringLiteral("segacd"), {QStringLiteral("genesis_plus_gx_libretro.so"), QStringLiteral("picodrive_libretro.so")}},
        {QStringLiteral("sega-saturn"),
         {QStringLiteral("mednafen_saturn_libretro.so"), QStringLiteral("yabause_libretro.so"), QStringLiteral("kronos_libretro.so")}},
        {QStringLiteral("saturn"),
         {QStringLiteral("mednafen_saturn_libretro.so"), QStringLiteral("yabause_libretro.so"), QStringLiteral("kronos_libretro.so")}},
        {QStringLiteral("dreamcast"), {QStringLiteral("flycast_libretro.so")}},

        // Arcade
        {QStringLiteral("mame"), {QStringLiteral("mame_libretro.so"), QStringLiteral("mame2016_libretro.so"), QStringLiteral("fbneo_libretro.so")}},
        {QStringLiteral("arcade"),
         {QStringLiteral("fbneo_libretro.so"),
          QStringLiteral("mame_libretro.so"),
          QStringLiteral("mame2016_libretro.so"),
          QStringLiteral("fbalpha2012_libretro.so")}},
        {QStringLiteral("cps1"), {QStringLiteral("fbneo_libretro.so"), QStringLiteral("fbalpha2012_libretro.so")}},
        {QStringLiteral("cps2"), {QStringLiteral("fbneo_libretro.so"), QStringLiteral("fbalpha2012_cps2_libretro.so")}},
        {QStringLiteral("cps3"), {QStringLiteral("fbneo_libretro.so"), QStringLiteral("fbalpha2012_cps3_libretro.so")}},
        {QStringLiteral("neogeo"), {QStringLiteral("fbneo_libretro.so"), QStringLiteral("fbalpha2012_neogeo_libretro.so")}},
        {QStringLiteral("neogeocd"), {QStringLiteral("neocd_libretro.so")}},
        {QStringLiteral("neogeoaes"), {QStringLiteral("fbneo_libretro.so"), QStringLiteral("geolith_libretro.so")}},

        // NEC PC Engine
        {QStringLiteral("pcengine"), {QStringLiteral("beetle_pce_libretro.so"), QStringLiteral("beetle_supergrafx_libretro.so")}},
        {QStringLiteral("turbografx-16"), {QStringLiteral("beetle_pce_libretro.so"), QStringLiteral("beetle_supergrafx_libretro.so")}},
        {QStringLiteral("pcenginecd"), {QStringLiteral("beetle_pce_libretro.so"), QStringLiteral("beetle_supergrafx_libretro.so")}},
        {QStringLiteral("turbografx-cd"), {QStringLiteral("beetle_pce_libretro.so"), QStringLiteral("beetle_supergrafx_libretro.so")}},
        {QStringLiteral("pcfx"), {QStringLiteral("beetle_pcfx_libretro.so")}},

        // Atari
        {QStringLiteral("atari2600"), {QStringLiteral("stella_libretro.so"), QStringLiteral("stella2014_libretro.so")}},
        {QStringLiteral("atari-2600"), {QStringLiteral("stella_libretro.so"), QStringLiteral("stella2014_libretro.so")}},
        {QStringLiteral("atari5200"), {QStringLiteral("atari800_libretro.so")}},
        {QStringLiteral("atari-5200"), {QStringLiteral("atari800_libretro.so")}},
        {QStringLiteral("atari7800"), {QStringLiteral("prosystem_libretro.so")}},
        {QStringLiteral("atari-7800"), {QStringLiteral("prosystem_libretro.so")}},
        {QStringLiteral("atarijaguar"), {QStringLiteral("virtualjaguar_libretro.so")}},
        {QStringLiteral("atari-jaguar"), {QStringLiteral("virtualjaguar_libretro.so")}},
        {QStringLiteral("atari800"), {QStringLiteral("atari800_libretro.so")}},

        // SNK
        {QStringLiteral("ngp"), {QStringLiteral("mednafen_ngp_libretro.so")}},
        {QStringLiteral("ngpc"), {QStringLiteral("mednafen_ngp_libretro.so")}},

        // Bandai
        {QStringLiteral("wswan"), {QStringLiteral("mednafen_wswan_libretro.so")}},
        {QStringLiteral("wonderswan"), {QStringLiteral("mednafen_wswan_libretro.so")}},

        // Other consoles
        {QStringLiteral("lynx"), {QStringLiteral("handy_libretro.so"), QStringLiteral("mednafen_lynx_libretro.so")}},
        {QStringLiteral("3do"), {QStringLiteral("opera_libretro.so")}},
        {QStringLiteral("colecovision"), {QStringLiteral("bluemsx_libretro.so")}},
        {QStringLiteral("intellivision"), {QStringLiteral("freeintv_libretro.so")}},
        {QStringLiteral("vectrex"), {QStringLiteral("vecx_libretro.so")}},
        {QStringLiteral("odyssey2"), {QStringLiteral("o2em_libretro.so")}},

        // Computers
        {QStringLiteral("dos"), {QStringLiteral("dosbox_pure_libretro.so"), QStringLiteral("dosbox_core_libretro.so")}},
        {QStringLiteral("amiga"), {QStringLiteral("puae_libretro.so"), QStringLiteral("puae2021_libretro.so")}},
        {QStringLiteral("c64"), {QStringLiteral("vice_x64_libretro.so")}},
        {QStringLiteral("c128"), {QStringLiteral("vice_x128_libretro.so")}},
        {QStringLiteral("zx-spectrum"), {QStringLiteral("fuse_libretro.so")}},
        {QStringLiteral("amstradcpc"), {QStringLiteral("cap32_libretro.so")}},
        {QStringLiteral("msx"), {QStringLiteral("bluemsx_libretro.so")}},
        {QStringLiteral("msx2"), {QStringLiteral("bluemsx_libretro.so")}},
        {QStringLiteral("pc-8800"), {QStringLiteral("quasi88_libretro.so")}},
        {QStringLiteral("pc-9800"), {QStringLiteral("np2kai_libretro.so")}},
    };
    return map;
}

QStringList retroarchCoreDirs(const QString &retroarchBinary)
{
    QString home = QDir::homePath();
    QStringList dirs;
    if (retroarchBinary == QStringLiteral("flatpak:org.libretro.RetroArch"))
        dirs << home + QStringLiteral("/.var/app/org.libretro.RetroArch/config/retroarch/cores");
    dirs << home + QStringLiteral("/.config/retroarch/cores");
    dirs << QStringLiteral("/usr/lib/x86_64-linux-gnu/libretro");
    dirs << QStringLiteral("/usr/lib/libretro");
    dirs << QStringLiteral("/usr/share/libretro/cores");
    return dirs;
}
