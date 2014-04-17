
#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "../common/bonanza_if.h"

using namespace godwhale::server;

int CONV server_iterate(int *value, move_t *pvseq, int *pvseq_length)
{
    std::vector<move_t> seq;
    int status;

    // Žè”Ô–ˆ‚ÌŽw‚µŽè‚ÌŒˆ’è‚ðs‚¢‚Ü‚·B
    status = Server::GetInstance()->Iterate(value, seq);
    
    for (int i = 0; i < seq.size(); ++i) {
        pvseq[i] = seq[i];
    }

    *pvseq_length = seq.size();
    return status;
}
