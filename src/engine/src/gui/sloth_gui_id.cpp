#include "sloth_gui_id.h"

namespace sloth
{
    static GuiId Fnv1aAppend(GuiId hash, const void* data, usize size)
    {
        const u8* bytes = static_cast<const u8*>(data);
        for (usize i = 0; i < size; ++i)
        {
            hash ^= bytes[i];
            hash *= 16777619u; // FNV prime (32-bit)
        }
        return hash;
    }

    GuiId HashGuiId(StringView label, GuiId seed)
    {
        return Fnv1aAppend(seed, label.Data(), label.Length());
    }

    GuiId HashGuiId(i32 value, GuiId seed)
    {
        return Fnv1aAppend(seed, &value, sizeof(value));
    }

    StringView StripGuiLabelHash(StringView label)
    {
        usize hashPos = label.IndexOf("##");
        return hashPos == StringView::npos ? label : label.Substring(0, hashPos);
    }

} // namespace sloth
