static void matrix_set_identity(float out[4][4]) {
    memset(out, 0, 4 * 4 * sizeof(float));
    out[0][0] = 1.0f;
    out[1][1] = 1.0f;
    out[2][2] = 1.0f;
    out[3][3] = 1.0f;
}

static void matrix_multiply(float out[4][4], float a[4][4], float b[4][4]) {
    int row = 0;
    int column = 0;

    while (row < 4) {
        column = 0;

        while (column < 4) {
            out[row][column] =
                b[0][column] * a[row][0]
                + b[1][column] * a[row][1]
                + b[2][column] * a[row][2]
                + b[3][column] * a[row][3]
                ;

            column += 1;
        }

        row += 1;
    }
}

// Construct a viewport transformation matrix
static void matrix_viewport(float out[4][4], float x, float y, float width, float height, float z_min, float z_max)
{
    matrix_set_identity(out);
    out[0][0] = width/2.0f;
    out[1][1] = height/-2.0f;
    out[2][2] = z_max - z_min;
    out[3][3] = 1.0f;
    out[3][0] = x + width/2.0f;
    out[3][1] = y + height/2.0f;
    out[3][2] = z_min;
}

// Construct a translation matrix from the given x-, y-, and z-component
static void matrix_translate(float out[4][4], float x, float y, float z) {
    matrix_set_identity(out);
    out[3][0] = x;
    out[3][1] = y;
    out[3][2] = z;
}

// Construct a scale matrix from the given x-, y-, and z-component
static void matrix_scale(float out[4][4], float x, float y, float z) {
    matrix_set_identity(out);
    out[0][0] = x;
    out[1][1] = y;
    out[2][2] = z;
}
