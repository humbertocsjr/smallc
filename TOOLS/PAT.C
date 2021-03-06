
/*
** pat.c -- pattern making and matching functions
*/

/*
** addset -- put c in set & increment j
*/
addset(c, set, j, maxsiz) char c, set[]; int *j, maxsiz; {
  if(*j >= maxsiz) return NO;
  set[*j]=c;
  *j = *j + 1;
  return YES;
  }

/*
** amatch -- look for match starting at lin[from]
*/
amatch(lin, from, pat) char lin[], pat[]; int from; {
  int i, j, offset, stack;
  stack = -1;
  offset=from;
  j=0;
  while(pat[j]!=NULL) {
    if(pat[j]==CLOSURE) {
      stack=j;
      j=j+CLOSIZE;
      i=offset;
      while(lin[i]!=NULL) {
        if(omatch(lin, &i, pat, j)==NO) break;
        }
      pat[stack+COUNT]=i-offset;
      pat[stack+START]=offset;
      offset=i;
      }
    else if(omatch(lin, &offset, pat, j)==NO) {
      while(stack >= 0) {
        if(pat[stack+COUNT] > 0) break;
        stack=pat[stack+PREVCL];
        }
      if(stack < 0) return -1;
      pat[stack+COUNT]=pat[stack+COUNT]-1;
      j=stack+CLOSIZE;
      offset=pat[stack+START]+pat[stack+COUNT];
      }
    j=j+patsiz(pat, j);
    }
  return offset;
  }

/*
** dodash -- expand array[i-1] - array[i+1] into set[j]...
*/
dodash(valid, array, i, set, j, maxset)
  char valid[], set[], array[]; int *i, *j, maxset; {
  int k, limit;
  *i = 1 + *i;
  *j = -1 + *j;
  limit=index(valid, esc(array, i));
  k=index(valid, set[*j]);
  while(k <= limit)
    addset(valid[k++], set, j, maxset);
  }

/*
** esc -- map array[i] into escaped char if appropriate
*/
esc(array, i) char array[]; int *i; {
  if(array[*i]!=ESCAPE) return array[*i];
  else if(array[ *i + 1]==NULL)    /* esc not special at end */
    return ESCAPE;
  else {
    *i= *i + 1;
    if(array[*i]=='n') return '\n';
    else if(array[*i]=='t') return '\t';
    else if(array[*i]=='b') return '\b';
    else if(array[*i]=='s') return ' ';
    else return array[*i];
    }
  }

/*
** filset -- expand set in array into set stopping at delim
*/
filset(delim, array, i, set, j, maxset)
  char delim, array[], set[]; int *i, *j, maxset; {
  char *digits, *lowalf, *upalf;
  digits="0123456789";
  lowalf="abcdefghijklmnopqrstuvwxyz";
  upalf="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  while((array[*i]!=delim)&(array[*i]!=NULL)) {
    if(array[*i]==ESCAPE)
      addset(esc(array, i), set, j, maxset);
    else if(array[*i]!='-')
      addset(array[*i], set, j, maxset);
    else if((j <= 0)|(array[*i+1]==NULL))    /* literal - */
      addset('-', set, j, maxset);
    else if(index(digits, set[*j -1]) > -1)
      dodash(digits, array, i, set, j, maxset);
    else if(index(lowalf, set[*j -1]) > -1)
      dodash(lowalf, array, i, set, j, maxset);
    else if(index(upalf, set[*j -1]) > -1)
      dodash(upalf, array, i, set, j, maxset);
    else addset('-', set, j, maxset);
    *i = *i + 1;
    }
  }

/*
** getccl -- expand char class at arg[i] into pat[j]
*/
getccl(arg, i, pat, j) char arg[], pat[]; int *i, *j; {
  int jstart;
  *i = *i + 1;  /**** skip over '[' in arg ****/
  if(arg[*i]==NOT) {
    addset(NCCL, pat, j, MAXPAT);
    *i = *i + 1;
    }
  else addset(CCL, pat, j, MAXPAT);
  jstart = *j;
  addset(0, pat, j, MAXPAT);  /**** leave room for count ****/
  filset(CCLEND, arg, i, pat, j, MAXPAT);
  pat[jstart] = *j - jstart - 1;
  if(arg[*i]==CCLEND) return YES;
  return ERR;
  }

/*
** locate -- look for c in char class at pat[offset]
*/
locate(c, pat, offset) char c, pat[]; int offset; {
  int i;
  /*
  ** size of class is at pat[offset], characters follow
  */
  i=offset+pat[offset];
  while( i > offset) {
    if(c==pat[i--]) return YES;
    }
  return NO;
  }

/*
** makpat -- make pattern from arg[from], end at delim
*/
makpat(arg, from, delim, pat) char arg[], delim, pat[]; int from; {
  int i, j, lastcl, lastj, lj;
  j=lastj=0;
  lastcl = -1;
  i=from;
  while((arg[i]!=delim)&(arg[i]!=NULL)) {
    lj=j;
    if(arg[i]==ANY) addset(ANY, pat, &j, MAXPAT);
    else if((arg[i]==BOL)&(i==from)) addset(BOL, pat, &j, MAXPAT);
    else if((arg[i]==EOL)&(arg[i+1]==delim)) addset(EOL, pat, &j, MAXPAT);
    else if(arg[i]==CCL) {
      if(getccl(arg, &i, pat, &j)==ERR) break;
      }
    else if((arg[i]==CLOSURE)&(i>from)) {
      lj=lastj;
      if((pat[lj]==BOL)|(pat[lj]==EOL)|(pat[lj]==CLOSURE)) break;
      lastcl=stclos(pat, &j, &lastj, lastcl);
      }
    else {
      addset(CHAR, pat, &j, MAXPAT);
      addset(esc(arg, &i), pat, &j, MAXPAT);
      }
    lastj=lj;
    ++i;
    }
  if((arg[i]!=delim)|(addset(NULL, pat, &j, MAXPAT)==NO)) return ERR;
  return i;
  }

/*
** match -- find match anywhere in line
*/
match(line, pattern) char line[], pattern[]; {
  int i;
  i=0;
  while(YES) {
    if(amatch(line, i, pattern) >= 0) return YES;
    if(line[i++]==NULL) return NO;
    }
  }

/*
** omatch -- try to match a single pattern at pat[j]
*/
omatch(lin, i, pat, j) char lin[], pat[]; int *i, j; {
  int bump;
  bump = -1;
  if(pat[j]==BOL) {
    if(*i==0) bump=0;
    }
  else if(pat[j]==EOL) {
    if(lin[*i]==NULL) bump=0;
    }
  else if(lin[*i]==NULL) return NO;
  else if(pat[j]==CHAR) {
    if(lin[*i]==pat[j+1]) bump=1;
    }
  else if(pat[j]==ANY) bump=1;
  else if(pat[j]==CCL) {
    if(locate(lin[*i], pat, j+1)==YES) bump=1;
    }
  else if(pat[j]==NCCL) {
    if(locate(lin[*i], pat, j+1)==NO) bump=1;
    }
  else error("in omatch: can't happen\n");
  if(bump >= 0) {
    *i = *i + bump;
    return YES;
    }
  return NO;
  }

/*
** patsiz -- returns size of entry at pat[n]
*/
patsiz(pat, n) char *pat; int n; {
  pat=pat+n;
  if(*pat==CHAR) return 2;
  else if((*pat==BOL)|(*pat==EOL)|(*pat==ANY)) return 1;
  else if((*pat==CCL)|(*pat==NCCL)) return (*(++pat)+2);
  else if(*pat==CLOSURE) return CLOSIZE;
  else error("in patsiz: can't happen\n");
  }

/*
** stclos -- insert closure entry at pat[j]
*/
stclos(pat, j, lastj, lastcl) char pat[]; int *j, *lastj, lastcl; {
  int jp, jt;
  jp = *j - 1;
  while(jp >= *lastj) {   /**** make hole for closure ****/
    jt = jp + CLOSIZE;
    addset(pat[jp--], pat, &jt, MAXPAT);
    }
  *j = *j + CLOSIZE;
  jp = *lastj;
  addset(CLOSURE, pat, lastj, MAXPAT);  /** CLOSURE **/
  addset(0, pat, lastj, MAXPAT);        /** COUNT **/
  addset(lastcl, pat, lastj, MAXPAT);   /** PREVCL **/
  addset(0, pat, lastj, MAXPAT);        /** START **/
  return jp;
  }
                                                                                                                                                                                                         