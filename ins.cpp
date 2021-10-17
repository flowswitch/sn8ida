#include "sn8.hpp"

instruc_t Instructions[] = {

{ "",           0                               },      // Unknown Operation

// data transfer

{ "=",     	CF_SFX|CF_CHG1|CF_USE2                 },
{ "XCH",     	CF_CHG1|CF_CHG2                 },
{ "MOVC",	CF_CHG1|CF_USE2			},

// arithmetic

{ "ADC",	CF_CHG1|CF_USE2			},
{ "+=",		CF_SFX|CF_CHG1|CF_USE2			},
{ "SBC",	CF_CHG1|CF_USE2			},
{ "-=",		CF_SFX|CF_CHG1|CF_USE2			},
{ "DAA",	CF_CHG1				},
{ "*=",		CF_SFX|CF_CHG1|CF_USE2			},

// logic

{ "&=",		CF_SFX|CF_CHG1|CF_USE2			},
{ "|=",		CF_SFX|CF_CHG1|CF_USE2			},
{ "^=",		CF_SFX|CF_CHG1|CF_USE2			},

// processing

{ "SWAP",	CF_CHG1|CF_USE2			},
{ "SWAP",	CF_CHG1				},
{ "RLC",	CF_CHG1|CF_USE2			},
{ "RLC",	CF_CHG1				},
{ "RRC",	CF_CHG1|CF_USE2			},
{ "RRC",	CF_CHG1				},
{ "=",		CF_SFX|CF_CHG1				},
{ "=",		CF_SFX|CF_CHG1				},
{ "=",		CF_SFX|CF_CHG1				},

// branch

{ "CMPSEQ",     CF_USE1|CF_USE2                 },     
{ "INCSZ",      CF_CHG1|CF_USE2                 },     
{ "INCSZ",	CF_CHG1				},
{ "DECSZ",      CF_CHG1|CF_USE2                 },     
{ "DECSZ",	CF_CHG1				},
{ "BTS",	CF_USE1|CF_USE2			},
{ "JMP",        CF_USE1|CF_STOP                 },    
{ "CALL",       CF_USE1|CF_CALL                 },     

// misc

{ "RET",        CF_STOP                 	},
{ "RETI",       CF_STOP                 	},
{ "NOP",	0				},
{ "PUSH",	0				},
{ "POP",	0				},

// pseudo
{ "IF",		CF_USE1				}, //IF EQ/NE
{ "IF",		CF_USE1|CF_USE2			}, //IF A<>55 / IF M.bit==0/1 / IF M++<>0
{ "IF",		CF_CHG1|CF_USE2|CF_USE3		}, //IF A=M++<>0

};

CASSERT(qnumber(Instructions) == SN8_last);
