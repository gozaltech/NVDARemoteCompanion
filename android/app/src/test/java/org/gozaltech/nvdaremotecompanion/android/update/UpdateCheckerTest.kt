package org.gozaltech.nvdaremotecompanion.android.update

import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.junit.Test

class UpdateCheckerTest {

    @Test
    fun `higher patch version is newer`() {
        assertTrue(UpdateChecker.isNewer("1.7.1", "1.7.0"))
    }

    @Test
    fun `higher minor version is newer`() {
        assertTrue(UpdateChecker.isNewer("1.8.0", "1.7.9"))
    }

    @Test
    fun `identical versions are not newer`() {
        assertFalse(UpdateChecker.isNewer("1.7.0", "1.7.0"))
    }

    @Test
    fun `older version is not newer`() {
        assertFalse(UpdateChecker.isNewer("1.6.9", "1.7.0"))
    }

    @Test
    fun `missing trailing components are treated as zero`() {
        assertFalse(UpdateChecker.isNewer("1.7", "1.7.0"))
        assertTrue(UpdateChecker.isNewer("1.7.1", "1.7"))
    }

    @Test
    fun `non numeric suffixes are ignored`() {
        assertTrue(UpdateChecker.isNewer("1.8.0-beta", "1.7.0"))
        assertFalse(UpdateChecker.isNewer("1.7.0-beta", "1.7.0"))
    }

    @Test
    fun `double digit components compare numerically not lexically`() {
        assertTrue(UpdateChecker.isNewer("1.10.0", "1.9.0"))
        assertFalse(UpdateChecker.isNewer("1.9.0", "1.10.0"))
    }
}
