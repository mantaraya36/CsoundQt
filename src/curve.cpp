/*
	Copyright (C) 2008, 2009 Andres Cabrera
			  (C) 2003 by John D. Ramsdell
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

#include "curve.h"

// Curve is a straightforward abstract data type for a curve

void Curve::copy(size_t size, MYFLT *data)
{
	// set_size must be called prior to this, as bounds are not checked.
	for (size_t i = 0; i < size; i++)
		m_data[i] = data[i];
}

void Curve::destroy()
{
	if (m_size != 0) {
		free(m_data);
	}
}

Curve::Curve(MYFLT *data, size_t size, const QString& caption,
			 Polarity polarity, MYFLT max, MYFLT min, MYFLT absmax,
			 MYFLT y_scale, bool dotted_divider, WINDAT *original)
	: m_caption(caption)
{
	m_size = 0;
	mutex.lock();
	set_size(size);
	copy(size, data);
	m_polarity = polarity;
	m_max = max;
	m_min = min;
	m_absmax = absmax;
	m_y_scale = y_scale;
	m_dotted_divider = dotted_divider;
	m_original = original;
    if(m_caption.contains("fft")) {
        m_curveType = CURVE_SPECTRUM;
    } else if(m_caption.contains("ftable")) {
        m_curveType = CURVE_FTABLE;
    } else {
        m_curveType = CURVE_AUDIOSIGNAL;
    }
	mutex.unlock();
    // qDebug() << "Curve " << m_curveType;

}

Curve::Curve(const Curve& curve)
	: m_caption(curve.m_caption)
{
	mutex.lock();
	set_size(curve.m_size);
	copy(curve.m_size, curve.m_data);
	m_polarity = curve.m_polarity;
	m_max = curve.m_max;
	m_min = curve.m_min;
	m_absmax = curve.m_absmax;
	m_y_scale = curve.m_y_scale;
	m_dotted_divider = curve.m_dotted_divider;
	mutex.unlock();
}

Curve& Curve::operator=(const Curve& curve)
{
	mutex.lock();
	if (this != &curve) {
		destroy();
		set_size(curve.m_size);
		copy(curve.m_size, curve.m_data);
		m_caption = curve.m_caption;
		m_polarity = curve.m_polarity;
		m_max = curve.m_max;
		m_min = curve.m_min;
		m_absmax = curve.m_absmax;
		m_y_scale = curve.m_y_scale;
		m_dotted_divider = curve.m_dotted_divider;
	}
	mutex.unlock();
	return *this;
}

Curve::~Curve()
{
	mutex.lock();
	destroy();
	mutex.unlock();
}

size_t Curve::get_size() const
{
	return m_size;
}

//uintptr_t Curve::get_id() const
//{
//  return m_id;
//}

// MYFLT *Curve::get_data() const
// {
//   return m_data;
// }

MYFLT Curve::get_data(int index)
{
	//   mutex.lock();
	MYFLT out = m_data[index];
	//   mutex.unlock();
	return out;
}

QString Curve::get_caption() const
{
	return m_caption;
}

Polarity Curve::get_polarity() const
{
	return m_polarity;
}

MYFLT Curve::get_max() const
{
	return m_max;
}

MYFLT Curve::get_min() const
{
	return m_min;
}

MYFLT Curve::get_absmax() const
{
	return m_absmax;
}

MYFLT Curve::get_y_scale() const
{
	return m_y_scale;
}

WINDAT * Curve::getOriginal()
{
	return m_original;
}

//void Curve::set_id(uintptr_t id)
//{
//  m_id = id;
//}

void Curve::set_data(MYFLT * data)
{
	copy(m_size, data);
}

void Curve::set_size(size_t size)
{
	if (m_size < size) { // This should happen only once on constructing the curve, as curves should change length
		if (m_size != 0) {
			free(m_data);
		}
		m_data = (MYFLT *) calloc(size, sizeof(MYFLT));
	}
	m_size = size;
}

void Curve::set_caption(QString caption)
{
    m_caption = caption.trimmed();
}

void Curve::set_polarity(Polarity polarity)
{
	m_polarity = polarity;
}

void Curve::set_max(MYFLT max)
{
	m_max = max;
}
void Curve::set_min(MYFLT min)
{
	m_min = min;
}

void Curve::set_absmax(MYFLT absmax)
{
	m_absmax = absmax;
}

void Curve::set_y_scale(MYFLT y_scale)
{
	m_y_scale = y_scale;
}

void Curve::setOriginal(WINDAT *windat)
{
	m_original = windat;
}

bool Curve::is_divider_dotted() const
{
	return m_dotted_divider;
}

bool Curve::has_same_caption(Curve *curve) const
{
	return curve && m_caption == curve->m_caption;
}

CurveType Curve::get_type() const
{
    return m_curveType;
}

QStringList parseCaption(QString caption) {
    QStringList out;
    if(caption.contains("fft")) {
        auto parts = caption.split(", ", Qt::SkipEmptyParts);
        auto p1 = parts[0].mid(6);  // skip "instr"
        auto signal = parts[1].mid(7);
        auto ffttype = parts[2].mid(5, parts[2].trimmed().size() - 2);
        out.append("fft");
        out.append(p1);
        out.append(signal);
        out.append(ffttype);
        return out;
    }
    else {
        out.append(caption);
        return out;
    }
}
