import h from 'snabbdom/h';
import Union from 'union-type';
import assoc from 'ramda/src/assoc';
import CodeMirror from 'codemirror';
import 'codemirror/lib/codemirror.css';
import 'codemirror/addon/scroll/simplescrollbars.css';
import './CodeView.scss';

require('codemirror/mode/clojure/clojure');
require('codemirror/mode/clike/clike');
require('codemirror/addon/scroll/simplescrollbars');

export const init = () => null;

export const Action = Union({
	InitCodeMirror: [Object],
	ViewCode: [String, String, Object]
});

export const update = Action.caseOn({
	InitCodeMirror: (codemirror, _) => codemirror,
	ViewCode: (code, mode, location, codemirror) => {
		codemirror.setOption("mode", mode);
		codemirror.setValue(code);
		codemirror.markText(
			{
				line: Math.max(0, location.start.line - 1),
				ch: Math.max(0, location.start.column - 1)
			},
			{
				line: Math.max(0, location.end.line - 1),
				ch: Math.max(0, location.end.column || 80)
			},
			{ className: "active-range" }
		);
		const t = codemirror.charCoords({line: location.start.line, ch: 0}, "local").top;
		const middleHeight = codemirror.getScrollerElement().offsetHeight / 2;
		codemirror.scrollTo(null, t - middleHeight - 5);
		return codemirror;
	}
});

export const render = (model, actions$) =>
	h("textarea", {
		hook: {
			insert: (vnode) => {
				const codemirror = CodeMirror.fromTextArea(vnode.elm, {
					theme: "default",
					mode: "clike",
					scrollbarStyle: "simple",
					lineNumbers: true,
					readOnly: true
				});
				actions$(Action.InitCodeMirror(codemirror));
			}
		}
	});
