"""Keep pinned JUCE 7 untouched; patch only a build-local copy for SDK 15+."""
from pathlib import Path
import argparse,shutil
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('destination',type=Path);a=p.parse_args()
marker=a.destination/'MAKKINE-SDK-COMPAT.txt'
if not marker.exists():
    shutil.copytree(a.source,a.destination,ignore=shutil.ignore_patterns('.git'),dirs_exist_ok=True)
    path=a.destination/'modules/juce_gui_basics/native/juce_Windowing_mac.mm'
    text=path.read_text()
    start=text.index('static Image createNSWindowSnapshot (NSWindow* nsWindow)')
    end=text.index('Image createSnapshotOfNativeWindow (void* nativeWindowHandle)',start)
    text=text[:start]+'''// Makkine SDK compatibility: cache this application's own NSView.
// CGWindowListCreateImage was removed from the macOS 15 SDK.
static Image createNSWindowSnapshot (NSWindow* nsWindow)
{
    JUCE_AUTORELEASEPOOL
    {
        NSView* view = [nsWindow contentView];
        if (view == nil) return {};
        NSBitmapImageRep* rep = [view bitmapImageRepForCachingDisplayInRect: [view bounds]];
        if (rep == nil || [rep pixelsWide] <= 0 || [rep pixelsHigh] <= 0) return {};
        [view cacheDisplayInRect: [view bounds] toBitmapImageRep: rep];
        Image result (Image::ARGB, (int) [rep pixelsWide], (int) [rep pixelsHigh], true);
        selectImageForDrawing (result);
        [rep drawInRect: NSMakeRect (0, 0, result.getWidth(), result.getHeight())];
        releaseImageAfterDrawing();
        return result;
    }
}

'''+text[end:]
    path.write_text(text)
    marker.write_text('JUCE 7.0.12 / build-local NSView snapshot compatibility patch. DSP unchanged.\n')
print(a.destination)
