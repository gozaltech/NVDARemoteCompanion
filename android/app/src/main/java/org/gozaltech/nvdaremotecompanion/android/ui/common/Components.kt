package org.gozaltech.nvdaremotecompanion.android.ui.common

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.selection.selectable
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.unit.dp
import kotlin.math.roundToInt

@Composable
fun SectionHeader(text: String) {
    Text(
        text = text,
        style = MaterialTheme.typography.titleMedium,
        modifier = Modifier.padding(top = 8.dp).semantics { heading() },
    )
}

@Composable
fun SwitchRow(
    label: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(label, modifier = Modifier.padding(end = 16.dp))
        Switch(checked = checked, onCheckedChange = onCheckedChange)
    }
}

@Composable
fun LabeledSlider(
    label: String,
    value: Float,
    range: ClosedFloatingPointRange<Float>,
    onValueCommitted: (Float) -> Unit,
) {
    var draft by remember(value) { mutableFloatStateOf(value) }
    val percent = percentOf(draft)

    Column(modifier = Modifier.fillMaxWidth()) {
        Text("$label: $percent%")
        Slider(
            value = draft,
            onValueChange = { draft = it },
            onValueChangeFinished = { onValueCommitted(draft) },
            valueRange = range,
            steps = stepsFor(range),
            modifier = Modifier.semantics { stateDescription = "$percent%" },
        )
    }
}

@Composable
fun OutlinedActionButton(text: String, onClick: () -> Unit) {
    OutlinedButton(onClick = onClick, modifier = Modifier.fillMaxWidth()) { Text(text) }
}

@Composable
fun ChoiceRow(label: String, selected: Boolean, onClick: () -> Unit) {
    Row(
        modifier =
            Modifier.fillMaxWidth()
                .selectable(selected = selected, role = Role.RadioButton, onClick = onClick)
                .padding(vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        RadioButton(selected = selected, onClick = null)
        Text(label, modifier = Modifier.padding(start = 8.dp))
    }
}

private const val PERCENT = 100f

private fun percentOf(value: Float): Int = (value * PERCENT).roundToInt()

private fun stepsFor(range: ClosedFloatingPointRange<Float>): Int =
    ((range.endInclusive - range.start) * PERCENT).roundToInt() - 1
