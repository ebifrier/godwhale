perl majority_server3.pl --client_port=4082 \
                        --client_num=1 \
                        --csa_host=wdoor.c.u-tokyo.ac.jp \
                        --csa_port=4081 \
                        --csa_id=godwhale \
                        --csa_pw=password01 \
                        --sec_limit=1500 \
                        --sec_limit_up=0 \
                        --time_response=0.15 \
                        --time_stable_min=11.0 \
                        --sec_spent_b=0 \
                        --sec_spent_w=0 \
                        --noaudio

#localhost \
#                        --csa_host=wdoor.c.u-tokyo.ac.jp \
#                        --csa_pw=test-900-0 \
#                        --final_as_confident
