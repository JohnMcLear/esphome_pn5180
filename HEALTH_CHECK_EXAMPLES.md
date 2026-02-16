# PN5180 Health Check Configuration Examples

## Example 1: Default Health Check (Recommended)
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  # Health check enabled by default with these settings:
  # - Checks every 60 seconds
  # - Auto-reset after 3 consecutive failures
  # - Declares unhealthy after 3 failures
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

---

## Example 2: Aggressive Health Monitoring
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  
  # Check more frequently
  health_check_interval: 30s
  
  # Be more sensitive to failures
  max_failed_checks: 2
  
  # Always try to auto-recover
  auto_reset_on_failure: true
  
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Use case**: Critical applications where quick failure detection is important

---

## Example 3: Conservative Health Monitoring
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  
  # Check less frequently to reduce overhead
  health_check_interval: 120s
  
  # Be more tolerant of temporary glitches
  max_failed_checks: 5
  
  # Auto-reset enabled (default)
  auto_reset_on_failure: true
  
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Use case**: Stable environments where false positives are a concern

---

## Example 4: Manual Recovery Only
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  
  health_check_interval: 60s
  max_failed_checks: 3
  
  # Disable auto-reset - manual intervention required
  auto_reset_on_failure: false
  
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Use case**: When you want to be notified of failures but handle recovery manually

---

## Example 5: Disabled Health Check
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  
  # Completely disable health checking
  health_check_enabled: false
  
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

**Use case**: Testing or when health monitoring is not needed

---

## Example 6: With Automations on Health Status Change
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  health_check_interval: 60s
  max_failed_checks: 3
  auto_reset_on_failure: true
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']

# Binary sensor will be auto-created for health status
# Note: Implementation may vary - this is conceptual
# You can monitor logs for health status changes
```

---

## Configuration Options Reference

| Option | Type | Default | Range | Description |
|--------|------|---------|-------|-------------|
| `health_check_enabled` | bool | `true` | - | Enable/disable health monitoring |
| `health_check_interval` | time | `60s` | 10s-300s | How often to check hardware health |
| `max_failed_checks` | int | `3` | 1-10 | Consecutive failures before declaring unhealthy |
| `auto_reset_on_failure` | bool | `true` | - | Automatically reset on failure threshold |

---

## Expected Log Output

### Healthy Operation
```
[I][pn5180:xxx]: Initial health check passed
[I][pn5180:xxx]: PN5180 setup complete
[V][pn5180:xxx]: Health check passed
```

### Degrading Health
```
[W][pn5180:xxx]: Health check failed (1/3 consecutive failures)
[W][pn5180:xxx]: Health check failed (2/3 consecutive failures)
[W][pn5180:xxx]: Health check failed (3/3 consecutive failures)
[E][pn5180:xxx]: PN5180 health check failed 3 times - reader may be unresponsive
[E][pn5180:xxx]: PN5180 declared unhealthy
```

### Auto-Recovery
```
[W][pn5180:xxx]: Attempting automatic reset...
[I][pn5180:xxx]: Reset successful - PN5180 responding again
[I][pn5180:xxx]: PN5180 health restored
```

### Failed Recovery
```
[W][pn5180:xxx]: Attempting automatic reset...
[E][pn5180:xxx]: Reset failed - PN5180 still not responding
```

---

## Health Check Implementation Details

### What the Health Check Does:
1. **BUSY Pin Verification**: Checks if BUSY pin is stuck (indicating hardware issue)
2. **SPI Communication Test**: Verifies SPI bus is responsive
3. **Consecutive Failure Tracking**: Counts failures to avoid false positives
4. **Auto-Recovery**: Attempts reset/re-initialization on threshold breach

### When Health Checks Run:
- Initial check during `setup()`
- Periodic checks in `loop()` based on `health_check_interval`
- Independent of `update_interval` (tag scanning)

### Health Status Effects:
- **Healthy**: Normal operation, tag scanning continues
- **Unhealthy**: Tag scanning skipped, logs indicate problem
- **Recovering**: After successful reset, status returns to healthy

---

## Troubleshooting Health Check Issues

### False Failures
**Symptom**: Health check fails occasionally but device works
**Solution**: 
- Increase `max_failed_checks` to 5 or higher
- Increase `health_check_interval` to reduce check frequency
- Check for EMI/noise on BUSY pin

### No Auto-Recovery
**Symptom**: Device declared unhealthy but doesn't recover
**Solution**:
- Verify `auto_reset_on_failure` is `true`
- Check physical connections (power, RST pin)
- May require manual restart (power cycle)

### Frequent Resets
**Symptom**: Device constantly resetting
**Solution**:
- Increase `max_failed_checks`
- Check power supply stability
- Verify antenna connection
- Disable `auto_reset_on_failure` and investigate root cause

---

## Integration with Home Assistant

Monitor health status via ESPHome logs:

```yaml
# In Home Assistant automations.yaml
automation:
  - alias: "NFC Reader Health Alert"
    trigger:
      - platform: state
        entity_id: binary_sensor.pn5180_status
        to: "unavailable"
    action:
      - service: notify.mobile_app
        data:
          message: "⚠️ NFC Reader unhealthy - check ESPHome logs"
```

---

## Best Practices

### For Production Use:
✅ Keep defaults (`health_check_interval: 60s`, `max_failed_checks: 3`)  
✅ Enable `auto_reset_on_failure`  
✅ Monitor logs for health events  
✅ Set up HA alerts for persistent failures  

### For Development/Testing:
✅ More aggressive checking (`health_check_interval: 30s`)  
✅ Lower failure threshold (`max_failed_checks: 2`)  
✅ Disable auto-reset to investigate issues  

### For Battery-Powered:
✅ Longer intervals (`health_check_interval: 120s`)  
✅ Consider disabling if power is critical  

---

## Performance Impact

Health checks are lightweight:
- **CPU**: Minimal (<0.1% overhead)
- **Memory**: ~100 bytes for tracking state
- **SPI Bus**: One quick check per interval
- **Tag Scanning**: No impact (runs independently)

The health check runs in `loop()`, not `update()`, so it doesn't interfere with normal tag scanning operations.
