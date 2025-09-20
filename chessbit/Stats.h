#pragma once

namespace stats {
	struct Stats {
		U64 caps;
		U64 eP;
		U64 cstl;
		U64 prom;
		U64 chk;
		U64 dischck;
		U64 dblchk;
		U64 chkm;
		U64 ttHits;

		constexpr Stats() :
			caps(0), eP(0), cstl(0), prom(0), chk(0), dischck(0), dblchk(0), chkm(0), ttHits(0) { }

		__forceinline constexpr Stats& operator+=(const Stats& s) {
			caps += s.caps;
			eP += s.eP;
			cstl += s.cstl;
			prom += s.prom;
			chk += s.chk;
			dischck += s.dischck;
			dblchk += s.dblchk;
			chkm += s.chkm;
			ttHits += s.ttHits;
			return *this;
		}
	};

	static inline Stats dummy;
}