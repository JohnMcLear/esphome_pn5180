# PN5180 Update Interval Testing Configurations

## Test 1: Aggressive (200ms - at minimum limit)
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  update_interval: 200ms  # Should work but show warning
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Expected:**
- ⚠️ Warning: "update_interval of 200ms is below recommended minimum"
- Should compile and run
- Fast scanning (5 scans/second)

---

## Test 2: Recommended Default (500ms)
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  update_interval: 500ms  # Recommended
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Expected:**
- ✅ No warnings
- Should compile and run
- Good balance (2 scans/second)

---

## Test 3: Conservative (1s)
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  update_interval: 1s  # Conservative, based on PN532 default
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Expected:**
- ✅ No warnings
- Should compile and run
- Power efficient (1 scan/second)

---

## Test 4: Very Conservative (5s)
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  update_interval: 5s  # Very slow
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Expected:**
- ✅ No warnings
- Should compile and run
- Very power efficient (0.2 scans/second)

---

## Test 5: Too Fast (100ms - below minimum)
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  update_interval: 100ms  # TOO FAST - should fail validation
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Expected:**
- ❌ Error: "value must be at least 200ms"
- Should NOT compile

---

## Test 6: Too Slow (15s - above maximum)
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  update_interval: 15s  # TOO SLOW - should fail validation
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Expected:**
- ❌ Error: "value must be at most 10s"
- Should NOT compile

---

## Test 7: No update_interval specified (use default)
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  # No update_interval - should use 500ms default
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Expected:**
- ✅ No warnings
- Uses default 500ms
- Should compile and run

---

## Hardware Testing Checklist

When testing with real hardware:

### Timing Verification
- [ ] 200ms: Count scans in 10 seconds (should be ~50)
- [ ] 500ms: Count scans in 10 seconds (should be ~20)
- [ ] 1s: Count scans in 10 seconds (should be ~10)
- [ ] 5s: Count scans in 50 seconds (should be ~10)

### Performance Impact
- [ ] Check CPU usage with different intervals
- [ ] Monitor free heap over time
- [ ] Test with other operations (LED, WiFi, etc)
- [ ] Look for timing jitter or delays

### Tag Detection
- [ ] Verify tags detected reliably at each interval
- [ ] Test rapid tap-and-remove (< 1 second)
- [ ] Test leaving tag on reader for extended time
- [ ] Check for false positives/negatives

### Edge Cases
- [ ] Multiple rapid tag changes
- [ ] Tag at edge of read range
- [ ] Reader with no tag for extended period
- [ ] System under heavy load

---

## Recommended Settings by Use Case

### Door Access Control
```yaml
update_interval: 200ms  # Fast response needed
```
- Quick detection when user approaches
- Warning acceptable for this use case

### Attendance Tracking
```yaml
update_interval: 500ms  # Default - good balance
```
- Fast enough for user experience
- No performance concerns

### Inventory/Asset Tracking
```yaml
update_interval: 1s  # Conservative
```
- Tags stay in place longer
- Power efficient

### Passive Monitoring
```yaml
update_interval: 5s  # Very conservative
```
- Battery powered applications
- Minimal power consumption

---

## Validation Rules Summary

| Setting | Value | Status |
|---------|-------|--------|
| Minimum | 200ms | Hard limit (error if below) |
| Recommended Min | 250ms | Soft limit (warning if below) |
| Default | 500ms | Recommended for most use cases |
| Maximum | 10s | Hard limit (error if above) |

## Expected Warnings

**Below 250ms (but ≥ 200ms):**
```
WARNING: PN5180: update_interval of 200ms is below recommended minimum of 250ms.
This may cause performance issues or interfere with other operations.
Consider using 500ms or higher for optimal stability.
```

**Below 200ms:**
```
ERROR: value must be at least 200ms
```

**Above 10s:**
```
ERROR: value must be at most 10s
```
