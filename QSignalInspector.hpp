/*! \file
 *
 * QSignalInspector
 *
 * https://github.com/j-ulrich/QSignalInspector
 *
 * \author Jochen Ulrich <jochenulrich@t-online.de>
 * \copyright
 * \parblock
 * © 2018 Jochen Ulrich
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * \endparblock
 */

#ifndef QSIGNALINSPECTOR_HPP
#define QSIGNALINSPECTOR_HPP

#include <QObject>
#include <QList>
#include <QSignalSpy>
#include <QMetaMethod>
#include <QDateTime>
#include <QSharedPointer>

/*! Struct representing one emission of a signal.
 */
struct QSignalEmissionEvent
{
	QMetaMethod signal;         //!< The QMetaMethod of the signal that was emitted.
	QDateTime timestamp;        //!< The time when the signal was emitted.
	QList<QVariant> parameters; //!< The parameter values of the emission.
};

/*! The QSignalInspector records signal emission of **all** signals of a class.
 *
 * QSignalInspector is very similar to QSignalSpy but it records the emission of all signals of a class.
 * Internally, QSignalInspector uses one QSignalSpy for each of the signals of the class.
 */
class QSignalInspector : public QObject, public QList<QSignalEmissionEvent>
{
	Q_OBJECT

public:


	/*! Creates a QSignalInspector recording signal emission of the given \p object.
	 *
	 * After a signal has been emitted by \p object, the information about the signal
	 * and the parameters of the emission can be accessed using the QList methods of this
	 * QSignalInspector.
	 *
	 * \param object The object whose signals should recorded.
	 * \param includeParentClassSignals If \c true, the signals of all parent classes of
	 * \p object are recorded as well. If \c false, only those signals are recorded that
	 * are declared by the last class in the inheritance hierarchy of \p object.
	 */
	explicit QSignalInspector(const QObject* object, bool includeParentClassSignals = true)
		: QObject()
	{
		const QMetaObject* const metaObject = object->metaObject();

		QMetaMethod signalEmittedSlot = staticMetaObject.method(staticMetaObject.indexOfSlot("signalEmitted()"));

		int methodIndex = includeParentClassSignals? 0 : metaObject->methodOffset();
		for (; methodIndex < metaObject->methodCount(); ++methodIndex)
		{
			QMetaMethod metaMethod = metaObject->method(methodIndex);
			if (metaMethod.methodType() == QMetaMethod::Signal)
			{
				m_signalSpies.insert(methodIndex, QSharedPointer<QSignalSpy>(new QSignalSpy(object, ("2"+metaMethod.methodSignature()).constData())));
				QObject::connect(object, metaMethod, this, signalEmittedSlot);
			}
		}
	}

private Q_SLOTS:
	void signalEmitted()
	{
		QObject* sender = this->sender();
		Q_ASSERT_X(sender, "signalEmitted()", "signalEmitted() slot must be called only via a signal");

		const QMetaObject* metaObject = sender->metaObject();

		/* For overloaded signals, senderSignalIndex() does not necessarily return the correct index.
		 * So we cannot rely on senderSignalIndex().
		 * Instead, we simply check all the signal spies until we find the one which caught the signal emission.
		 */
		int signalIndex = 0;
		QSharedPointer<QSignalSpy> signalSpy;
		for (; signalIndex < metaObject->methodCount(); ++signalIndex)
		{
			if (metaObject->method(signalIndex).methodType() != QMetaMethod::Signal)
				continue;

			signalSpy = m_signalSpies.value(signalIndex);
			if (!signalSpy)
			{
				qWarning("QSignalInspector: Unexpected signal emitted");
				return;
			}
			if (signalSpy->count() > 0)
				break;
		}

		QList<QVariant> signalParameters = signalSpy->at(0);
		QSignalEmissionEvent event;
		event.signal = metaObject->method(signalIndex);
		event.timestamp = QDateTime::currentDateTime();
		event.parameters = signalParameters;
		this->append(event);
		signalSpy->clear();
	}

private:
	QMap<int, QSharedPointer<QSignalSpy> > m_signalSpies;
};

#endif /* QSIGNALINSPECTOR_HPP */
