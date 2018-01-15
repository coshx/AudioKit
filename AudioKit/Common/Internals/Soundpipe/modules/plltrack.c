//
//  plltrack.c
//  AudioKit
//
//  Created by Michael Holroyd on 1/15/18.
//  Copyright © 2018 AudioKit. All rights reserved.
//

#include <stdio.h>

/*
 * PllTrack
 *
 * This code has been extracted from the Csound opcode "plltrack".
 * It has been modified to work as a Soundpipe module.
 *
 * Original Author(s): Victor Lazzarini, Miller Puckette (Original Algorithm)
 * Year: 2007
 * Location: Opcodes/pitchtrack.c
 *
 */


#include <stdlib.h>
#include <math.h>
#include "soundpipe.h"

#define ROOT2 (1.4142135623730950488)
enum {LP1=0, LP2, HP};

void update_coefs(sp_plltrack *p, double fr, double Q, BIQUAD *biquad, int TYPE)
{
    double k, ksq, div, ksqQ;
    
    switch(TYPE){
        case LP2:
            k = tan(fr*p->pidsr);
            ksq = k*k;
            ksqQ = ksq*Q;
            div = ksqQ+k+Q;
            biquad->b1 = (2*Q*(ksq-1.))/div;
            biquad->b2 = (ksqQ-k+Q)/div;
            biquad->a0 = ksqQ/div;
            biquad->a1 = 2*biquad->a0;
            biquad->a2 = biquad->a0;
            break;
            
        case LP1:
            k = 1.0/tan(p->pidsr*fr);
            ksq = k*k;
            biquad->a0 = 1.0 / ( 1.0 + ROOT2 * k + ksq);
            biquad->a1 = 2.0*biquad->a0;
            biquad->a2 = biquad->a0;
            biquad->b1 = 2.0 * (1.0 - ksq) * biquad->a0;
            biquad->b2 = ( 1.0 - ROOT2 * k + ksq) * biquad->a0;
            break;
            
        case HP:
            k = tan(p->pidsr*fr);
            ksq = k*k;
            biquad->a0 = 1.0 / ( 1.0 + ROOT2 * k + ksq);
            biquad->a1 = -2.*biquad->a0;
            biquad->a2 = biquad->a0;
            biquad->b1 = 2.0 * (ksq - 1.0) * biquad->a0;
            biquad->b2 = ( 1.0 - ROOT2 * k + ksq) * biquad->a0;
            break;
    }
    
}


int plltrack_set(sp_plltrack *p)
{
    int i;
    p->x1 = p->cos_x = p->sin_x = 0.0;
    p->x2 = 1.0;
    p->klpf_o = p->klpfQ_o = p->klf_o = p->khf_o = 0.0;
    update_coefs(p,10.0, 0.0, &p->fils[4], LP1);
    p->ace = p->xce = 0.0;
    for (i=0; i < 6; i++)
        p->fils[i].del1 = p->fils[i].del2 = 0.0;
    
    return SP_OK;
}

int plltrack_perf(sp_plltrack *p)
{
    int ksmps, i, k;
    SPFLOAT _0dbfs;
    double a0[6], a1[6], a2[6], b1[6], b2[6];
    double *mem1[6], *mem2[6];
    double *ace, *xce;
    double *cos_x, *sin_x, *x1, *x2;
    double scal,esr;
    BIQUAD *biquad = p->fils;
    SPFLOAT *asig=p->asig,kd=*p->kd,klpf,klpfQ,klf,khf,kthresh;
    SPFLOAT *freq=p->freq, *lock =p->lock, itmp = asig[0];
    int itest = 0;
    
    _0dbfs = p->e0dbfs;
    ksmps = CS_KSMPS;
    esr = CS_ESR;
    scal = 2.0*p->pidsr;
    
    /* check for muted input & bypass */
    if (ksmps > 1){
        for (i=0; i < ksmps; i++) {
            if (asig[i] != 0.0 && asig[i] != itmp) {
                itest = 1;
                break;
            }
            itmp = asig[i];
        }
        if (!itest)  return SP_OK;
    } else if (*asig == 0.0) return SP_OK;
    
    
    if (*p->klpf == 0) klpf = 20.0;
    else klpf = *p->klpf;
    
    if (*p->klpfQ == 0) klpfQ =  1./3.;
    else klpfQ = *p->klpfQ;
    
    if (*p->klf == 0) klf = 20.0;
    else klf = *p->klf;
    
    if (*p->khf == 0) khf = 1500.0;
    else khf = *p->khf;
    
    if (*p->kthresh == 0.0) kthresh= 0.001;
    else kthresh = *p->kthresh;
    
    
    
    if (p->khf_o != khf) {
        update_coefs(p, khf, 0.0, &biquad[0], LP1);
        update_coefs(p, khf, 0.0, &biquad[1], LP1);
        update_coefs(p, khf, 0.0, &biquad[2], LP1);
        p->khf_o = khf;
    }
    
    if (p->klf_o != klf) {
        update_coefs(csound, klf, 0.0, &biquad[3], HP);
        p->klf_o = klf;
    }
    
    if (p->klpf_o != klpf || p->klpfQ_o != klpfQ ) {
        update_coefs(csound, klpf, klpfQ, &biquad[5], LP2);
        p->klpf_o = klpf; p->klpfQ_o = klpfQ;
    }
    
    for (k=0; k < 6; k++) {
        a0[k] = biquad[k].a0;
        a1[k] = biquad[k].a1;
        a2[k] = biquad[k].a2;
        b1[k] = biquad[k].b1;
        b2[k] = biquad[k].b2;
        mem1[k] = &(biquad[k].del1);
        mem2[k] = &(biquad[k].del2);
    }
    
    cos_x = &p->cos_x;
    sin_x = &p->sin_x;
    x1 = &p->x1;
    x2 = &p->x2;
    xce = &p->xce;
    ace = &p->ace;
    
    for (i=0; i < ksmps; i++){
        double input = (double) (asig[i]/_0dbfs), env;
        double w, y, icef = 0.99, fosc, xd, c, s, oc;
        
        /* input stage filters */
        for (k=0; k < 4 ; k++){
            w =  input - *(mem1[k])*b1[k] - *(mem2[k])*b2[k];
            y  = w*a0[k] + *(mem1[k])*a1[k] + *(mem2[k])*a2[k];
            *(mem2[k]) = *(mem1[k]);
            *(mem1[k]) = w;
            input = y;
        }
        
        /* envelope extraction */
        w =  FABS(input) - *(mem1[k])*b1[k] - *(mem2[k])*b2[k];
        y  = w*a0[k] + *(mem1[k])*a1[k] + *(mem2[k])*a2[k];
        *(mem2[k]) = *(mem1[k]);
        *(mem1[k]) = w;
        env = y;
        k++;
        
        /* constant envelope */
        if (env > kthresh)
            input /= env;
        else input = 0.0;
        
        /*post-ce filter */
        *ace = (1.-icef)*(input + *xce)/2. + *ace*icef;
        *xce = input;
        
        /* PLL */
        xd =  *cos_x * (*ace) * kd * esr;
        w =  xd - *(mem1[k])*b1[k] - *(mem2[k])*b2[k];
        y  = w*a0[k] + *(mem1[k])*a1[k] + *(mem2[k])*a2[k];
        *(mem2[k]) = *(mem1[k]);
        *(mem1[k]) = w;
        freq[i] = FABS(2*y);
        lock[i] = *ace * (*sin_x);
        fosc = y + xd;
        
        /* quadrature osc */
        *sin_x = *x1;
        *cos_x = *x2;
        oc = fosc*scal;
        c = COS(oc);  s = SIN(oc);
        *x1 = *sin_x*c + *cos_x*s;
        *x2 = -*sin_x*s + *cos_x*c;
        
    }
    return SP_OK;
}
