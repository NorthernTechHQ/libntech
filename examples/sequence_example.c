#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <libutils/sequence.h>

int main() {
    Seq *seq = SeqNew(2, free);
    SeqAppend(seq, strdup("Hello"));
    SeqAppend(seq, strdup("world!"));

    size_t len = SeqLength(seq);
    for (size_t i = 0; i < len; i++) {
        printf("%s ", SeqAt(seq, i));
    }
    printf("\n");
    SeqDestroy(seq);

    return 0;
}
