import React, { useState } from 'react';

export default function VitalsChart({ readings, title, metricKey, unit, color, threshold, thresholdLabel }) {
  const [timeRange, setTimeRange] = useState('1h');

  // Filter valid data points for this specific metric
  const validData = (readings || [])
    .filter(r => r[metricKey] != null)
    .map(r => ({
      timestamp: new Date(r.timestamp),
      value: r[metricKey]
    }))
    .reverse(); // Chronological order

  if (validData.length === 0) {
    return (
      <div className="chart-card">
        <div className="chart-header">
          <span className="chart-title">{title}</span>
        </div>
        <div style={{
          height: '180px',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          color: 'var(--text-dim)',
          fontStyle: 'italic',
          background: 'rgba(15, 23, 42, 0.4)',
          borderRadius: '6px'
        }}>
          No readings available for this period.
        </div>
      </div>
    );
  }

  // Calculate scaling boundaries
  const values = validData.map(d => d.value);
  const minVal = Math.min(...values, threshold ? threshold * 0.9 : 0);
  const maxVal = Math.max(...values, threshold ? threshold * 1.1 : 100);
  const valRange = maxVal - minVal || 1;

  const width = 600;
  const height = 180;
  const padding = { top: 20, right: 30, bottom: 30, left: 45 };
  const graphWidth = width - padding.left - padding.right;
  const graphHeight = height - padding.top - padding.bottom;

  // Generate SVG polyline points
  const points = validData.map((d, i) => {
    const x = padding.left + (i / (validData.length - 1 || 1)) * graphWidth;
    const y = padding.top + graphHeight - ((d.value - minVal) / valRange) * graphHeight;
    return `${x},${y}`;
  }).join(' ');

  // Calculate threshold line Y
  let thresholdY = null;
  if (threshold && threshold >= minVal && threshold <= maxVal) {
    thresholdY = padding.top + graphHeight - ((threshold - minVal) / valRange) * graphHeight;
  }

  const latestVal = validData[validData.length - 1].value;

  return (
    <div className="chart-card">
      <div className="chart-header">
        <div>
          <span className="chart-title">{title}</span>
          <span style={{ marginLeft: '0.75rem', fontFamily: 'var(--font-mono)', fontWeight: 700, color }}>
            Latest: {latestVal} {unit}
          </span>
        </div>
        <div style={{ display: 'flex', gap: '0.35rem' }}>
          {['1h', '6h', '24h'].map(t => (
            <button
              key={t}
              onClick={() => setTimeRange(t)}
              style={{
                background: timeRange === t ? 'var(--border-color)' : 'transparent',
                border: '1px solid var(--border-color)',
                color: timeRange === t ? '#fff' : 'var(--text-dim)',
                padding: '0.2rem 0.5rem',
                fontSize: '0.7rem',
                borderRadius: '4px',
                cursor: 'pointer'
              }}
            >
              {t}
            </button>
          ))}
        </div>
      </div>

      <div className="chart-svg-container">
        <svg viewBox={`0 0 ${width} ${height}`} style={{ width: '100%', height: '100%' }}>
          {/* Grid lines */}
          <line
            x1={padding.left} y1={padding.top}
            x2={width - padding.right} y2={padding.top}
            stroke="#334155" strokeDasharray="3,3" strokeWidth="1"
          />
          <line
            x1={padding.left} y1={padding.top + graphHeight / 2}
            x2={width - padding.right} y2={padding.top + graphHeight / 2}
            stroke="#334155" strokeDasharray="3,3" strokeWidth="1"
          />
          <line
            x1={padding.left} y1={padding.top + graphHeight}
            x2={width - padding.right} y2={padding.top + graphHeight}
            stroke="#334155" strokeWidth="1"
          />

          {/* Threshold reference line */}
          {thresholdY !== null && (
            <g>
              <line
                x1={padding.left} y1={thresholdY}
                x2={width - padding.right} y2={thresholdY}
                stroke="#ef4444" strokeDasharray="4,4" strokeWidth="1.5"
              />
              <text
                x={width - padding.right - 5} y={thresholdY - 5}
                fill="#ef4444" fontSize="10" textAnchor="end" fontWeight="600"
              >
                {thresholdLabel || `Limit: ${threshold}`}
              </text>
            </g>
          )}

          {/* Y Axis labels */}
          <text x={padding.left - 8} y={padding.top + 4} fill="#64748b" fontSize="10" textAnchor="end">
            {maxVal.toFixed(0)}
          </text>
          <text x={padding.left - 8} y={padding.top + graphHeight} fill="#64748b" fontSize="10" textAnchor="end">
            {minVal.toFixed(0)}
          </text>

          {/* Data Line */}
          <polyline
            fill="none"
            stroke={color}
            strokeWidth="2.5"
            strokeLinecap="round"
            strokeLinejoin="round"
            points={points}
          />

          {/* Data point dots */}
          {validData.map((d, i) => {
            const x = padding.left + (i / (validData.length - 1 || 1)) * graphWidth;
            const y = padding.top + graphHeight - ((d.value - minVal) / valRange) * graphHeight;
            return (
              <circle
                key={i}
                cx={x} cy={y} r="3"
                fill={color}
                stroke="var(--bg-card)" strokeWidth="1.5"
              />
            );
          })}
        </svg>
      </div>
    </div>
  );
}
