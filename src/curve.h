/*
	Copyright (C) 2008, 2009 Andres Cabrera
			  (C) 2003 by John D. Ramsdell.
	mantaraya36@gmail.com

	This file is part of CsoundQt.

	CsoundQt is free software; you can redistribute it
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	CsoundQt is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with Csound; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
	02111-1307 USA
*/

#ifndef CURVE_H
#define CURVE_H

//#include <QString>
#include <QMutex>
#include "types.h"
#include <csound_graph_display.h>

enum Polarity {
	POLARITY_NOPOL,
	POLARITY_NEGPOL,
	POLARITY_POSPOL,
	POLARITY_BIPOL
};

enum CurveType {
    CURVE_FTABLE=1,
    CURVE_AUDIOSIGNAL=2,
    CURVE_SPECTRUM=3
};


class Curve
{
public:
	Curve(MYFLT *, size_t, const QString&, Polarity,
		  MYFLT, MYFLT, MYFLT, MYFLT, bool, WINDAT *original);
	Curve(const Curve&);
	Curve &operator=(const Curve&);
	~Curve();
	//    uintptr_t get_id() const;
	//     MYFLT *get_data() const;
	MYFLT get_data(int index);
	size_t get_size() const;      // number of points
    QString get_caption() const; // original caption of curve
    Polarity get_polarity() const; // polarity
	MYFLT get_max() const;        // curve max
	MYFLT get_min() const;        // curve min
	MYFLT get_absmax() const;     // abs max of above
	MYFLT get_y_scale() const;    // Y axis scaling factor
    CurveType get_type() const;
	WINDAT * getOriginal();

	//    void set_id(uintptr_t id);
	void set_data(MYFLT * data);
	void set_size(size_t size);      // number of points
	void set_caption(QString caption); // title of curve
    void set_polarity(Polarity polarity); // polarity
	void set_max(MYFLT max);        // curve max
	void set_min(MYFLT min);        // curve min
	void set_absmax(MYFLT absmax);     // abs max of above
	void set_y_scale(MYFLT y_scale);    // Y axis scaling factor
	void setOriginal(WINDAT *windat);

	bool is_divider_dotted() const; // Add dotted divider when true
	bool has_same_caption(Curve *) const;
private:
	//    uintptr_t m_id;
	MYFLT *m_data;
	WINDAT *m_original;
	size_t m_size;
	QString m_caption;
    QString m_title;
    Polarity m_polarity;
	MYFLT m_max, m_min, m_absmax, m_y_scale;
	bool m_dotted_divider;
	void copy(size_t, MYFLT *);
	void destroy();
    CurveType m_curveType;

	QMutex mutex;
};

#endif
