struct color
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

static inline struct color
color(u8 r, u8 g, u8 b, u8 a)
{
    struct color result;
    result.r = r;
    result.g = g;
    result.b = b;
    result.a = a;
    return result;
}

static const struct color BASE_COLORS[] = {
    { 0x28, 0x28, 0x28, 0xFF },
    { 0x2D, 0x99, 0x99, 0xFF },
    { 0x99, 0x99, 0x2D, 0xFF },
    { 0x99, 0x2D, 0x99, 0xFF },
    { 0x2D, 0x99, 0x51, 0xFF },
    { 0x99, 0x2D, 0x2D, 0xFF },
    { 0x2D, 0x63, 0x99, 0xFF },
    { 0x99, 0x63, 0x2D, 0xFF },
    { 0x00, 0x23, 0x66, 0xFF },
};

static const struct color LIGHT_COLORS[] = {
    { 0x28, 0x28, 0x28, 0xFF },
    { 0x44, 0xE5, 0xE5, 0xFF },
    { 0xE5, 0xE5, 0x44, 0xFF },
    { 0xE5, 0x44, 0xE5, 0xFF },
    { 0x44, 0xE5, 0x7A, 0xFF },
    { 0xE5, 0x44, 0x44, 0xFF },
    { 0x44, 0x95, 0xE5, 0xFF },
    { 0xE5, 0x95, 0x44, 0xFF },
    { 0x00, 0x63, 0x99, 0xFF },
};

static const struct color DARK_COLORS[] = {
    { 0x28, 0x28, 0x28, 0xFF },
    { 0x1E, 0x66, 0x66, 0xFF },
    { 0x66, 0x66, 0x1E, 0xFF },
    { 0x66, 0x1E, 0x66, 0xFF },
    { 0x1E, 0x66, 0x36, 0xFF },
    { 0x66, 0x1E, 0x1E, 0xFF },
    { 0x1E, 0x42, 0x66, 0xFF },
    { 0x66, 0x42, 0x1E, 0xFF },
    { 0x00, 0x00, 0x20, 0xFF },
};
