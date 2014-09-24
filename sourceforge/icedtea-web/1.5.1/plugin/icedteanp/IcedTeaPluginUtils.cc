/* IcedTeaPluginUtils.cc

   Copyright (C) 2009, 2010  Red Hat

This file is part of IcedTea.

IcedTea is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

IcedTea is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with IcedTea; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA.

Linking this library statically or dynamically with other modules is
making a combined work based on this library.  Thus, the terms and
conditions of the GNU General Public License cover the whole
combination.

As a special exception, the copyright holders of this library give you
permission to link this library with independent modules to produce an
executable, regardless of the license terms of these independent
modules, and to copy and distribute the resulting executable under
terms of your choice, provided that you also meet, for each linked
independent module, the terms and conditions of the license of that
module.  An independent module is a module which is not derived from
or based on this library.  If you modify this library, you may extend
this exception to your version of the library, but you are not
obligated to do so.  If you do not wish to do so, delete this
exception statement from your version. */

#include "IcedTeaNPPlugin.h"
#include "IcedTeaScriptablePluginObject.h"
#include "IcedTeaPluginUtils.h"
#include <fstream>

/**
 * Misc. utility functions used by the plugin
 */

/***********************************************
 * Begin IcedTeaPluginUtilities implementation *
************************************************/

// Initialize static variables
int IcedTeaPluginUtilities::reference = -1;
pthread_mutex_t IcedTeaPluginUtilities::reference_mutex = PTHREAD_MUTEX_INITIALIZER;
std::map<void*, NPP>* IcedTeaPluginUtilities::instance_map = new std::map<void*, NPP>();
std::map<std::string, NPObject*>* IcedTeaPluginUtilities::object_map = new std::map<std::string, NPObject*>();
std::queue<std::string> pre_jvm_message;

/* Plugin async call queue */
static std::vector< PluginThreadCall* >* pendingPluginThreadRequests = new std::vector< PluginThreadCall* >();

void *flush_pre_init_messages(void* data) {
  while (true){
    struct timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    nanosleep(&ts ,0);
    if (jvm_up) {
      while (!pre_jvm_message.empty()) {
        pthread_mutex_lock(&debug_pipe_lock);
        std::string message = pre_jvm_message.front();
        pre_jvm_message.pop();
        pthread_mutex_unlock(&debug_pipe_lock);
        plugin_send_message_to_appletviewer_console(message.c_str());
      }
      flush_plugin_send_message_to_appletviewer_console();
    }
  }
  return NULL;
}

void push_pre_init_messages(char * ldm){
  pthread_mutex_lock(&debug_pipe_lock);
  pre_jvm_message.push(std::string(ldm));
  pthread_mutex_unlock(&debug_pipe_lock);
}

void reset_pre_init_messages(){
    pre_jvm_message = std::queue<std::string>();
  }

/**
 * Given a context number, constructs a message prefix to send to Java
 *
 * @param context The context of the request
 * @return The string prefix (allocated on heap)
 */

void
IcedTeaPluginUtilities::constructMessagePrefix(int context, std::string *result)
{
	std::string context_str = std::string();

	itoa(context, &context_str);

	result->append("context ");
	result->append(context_str);
	result->append(" reference -1");

}

/**
 * Given a context number, and reference number, constructs a message prefix to
 * send to Java
 *
 * @param context The context of the request
 * @param rerefence The reference number of the request
 * @param result The message
 */

void
IcedTeaPluginUtilities::constructMessagePrefix(int context, int reference, std::string* result)
{
    // Until security is implemented, use file:// source for _everything_

	std::string context_str = std::string();
	std::string reference_str = std::string();

	itoa(context, &context_str);
	itoa(reference, &reference_str);

	*result += "context ";
	result->append(context_str);
	*result += " reference ";
	result->append(reference_str);
}

/**
 * Given a context number, reference number, and source location, constructs
 * a message prefix to send to Java
 *
 * @param context The context of the request
 * @param rerefence The reference number of the request
 * @param address The address for the script that made the request
 * @param result The message
 */

void
IcedTeaPluginUtilities::constructMessagePrefix(int context, int reference,
		                                       std::string address,
		                                       std::string* result)
{
	std::string context_str = std::string();
	std::string reference_str = std::string();

	itoa(context, &context_str);
	itoa(reference, &reference_str);

	*result += "context ";
	result->append(context_str);
	*result += " reference ";
	result->append(reference_str);

	if (address.length() > 0)
	{
	    *result += " src ";
        result->append(address);
	}
}

/**
 * Returns a string representation of a void pointer
 *
 * @param id The pointer
 * @param result The string representation
 */

void
IcedTeaPluginUtilities::JSIDToString(void* id, std::string* result)
{
	char id_str[NUM_STR_BUFFER_SIZE];

	if (sizeof(void*) == sizeof(long long))
	{
		snprintf(id_str, NUM_STR_BUFFER_SIZE, "%llu", id);
	}
	else
	{
		snprintf(id_str, NUM_STR_BUFFER_SIZE, "%lu", id); // else use long
	}

	result->append(id_str);

	PLUGIN_DEBUG("Converting pointer %p to %s\n", id, id_str);
}

/**
 * Returns a void pointer from a string representation
 *
 * @param id_str The string representation
 * @return The pointer
 */

void*
IcedTeaPluginUtilities::stringToJSID(std::string id_str)
{
	void* ptr;
	if (sizeof(void*) == sizeof(long long))
	{
		PLUGIN_DEBUG("Casting (long long) \"%s\" -- %llu\n", id_str.c_str(), strtoull(id_str.c_str(), NULL, 0));
		ptr = reinterpret_cast <void*> ((unsigned long long) strtoull(id_str.c_str(), NULL, 0));
	} else
	{
		PLUGIN_DEBUG("Casting (long) \"%s\" -- %lu\n", id_str.c_str(), strtoul(id_str.c_str(), NULL, 0));
		ptr = reinterpret_cast <void*> ((unsigned long)  strtoul(id_str.c_str(), NULL, 0));
	}

	PLUGIN_DEBUG("Casted: %p\n", ptr);

	return ptr;
}

/**
 * Returns a void pointer from a string representation
 *
 * @param id_str The pointer to the string representation
 * @return The pointer
 */

void*
IcedTeaPluginUtilities::stringToJSID(std::string* id_str)
{
    void* ptr;
    if (sizeof(void*) == sizeof(long long))
    {
        PLUGIN_DEBUG("Casting (long long) \"%s\" -- %llu\n", id_str->c_str(), strtoull(id_str->c_str(), NULL, 0));
        ptr = reinterpret_cast <void*> ((unsigned long long) strtoull(id_str->c_str(), NULL, 0));
    } else
    {
        PLUGIN_DEBUG("Casting (long) \"%s\" -- %lu\n", id_str->c_str(), strtoul(id_str->c_str(), NULL, 0));
        ptr = reinterpret_cast <void*> ((unsigned long)  strtoul(id_str->c_str(), NULL, 0));
    }

    PLUGIN_DEBUG("Casted: %p\n", ptr);

    return ptr;
}

/**
 * Increments the global reference number and returns it.
 *
 * This function is thread-safe.
 */
int
IcedTeaPluginUtilities::getReference()
{
	pthread_mutex_lock(&reference_mutex);

	// If we are nearing the max, reset
	if (reference < -0x7FFFFFFF + 10) {
	    reference = -1;
	}

	reference--;
	pthread_mutex_unlock(&reference_mutex);

	return reference;
}

/**
 * Decrements the global reference number.
 *
 * This function is thread-safe.
 */
void
IcedTeaPluginUtilities::releaseReference()
{
    // do nothing for now
}

/**
 * Converts integer to char*
 *
 * @param i The integer to convert to ascii
 * @param result The resulting string
 */
void
IcedTeaPluginUtilities::itoa(int i, std::string* result)
{
	char int_str[NUM_STR_BUFFER_SIZE];
	snprintf(int_str, NUM_STR_BUFFER_SIZE, "%d", i);
	result->append(int_str);
}

/**
 * Frees memory from a string* vector
 *
 * The vector deconstructor will only delete string pointers upon being
 * called. This function frees the associated string memory as well.
 *
 * @param v The vector whose strings are to be freed
 */
void
IcedTeaPluginUtilities::freeStringPtrVector(std::vector<std::string*>* v)
{
	if (v)
	{
		for (int i=0; i < v->size(); i++) {
			delete v->at(i);
		}

		delete v;
	}

}

/**
 * Given a string, splits it on the given delimiters.
 *
 * @param str The string to split
 * @param The delimiters to split on
 * @return A string vector containing the split components
 */

std::vector<std::string*>*
IcedTeaPluginUtilities::strSplit(const char* str, const char* delim)
{
	std::vector<std::string*>* v = new std::vector<std::string*>();
	v->reserve(strlen(str)/2);
	char* copy;

	// Tokenization is done on a copy
	copy = (char*) malloc (sizeof(char)*strlen(str) + 1);
	strcpy(copy, str);

	char* tok_ptr;
	tok_ptr = strtok (copy, delim);

	while (tok_ptr != NULL)
	{
	    // Allocation on heap since caller has no way to knowing how much will
	    // be needed. Make sure caller cleans up!
	    std::string* s = new std::string();
	    s->append(tok_ptr);
	    v->push_back(s);
	    tok_ptr = strtok (NULL, delim);
	}
        free(copy);

	return v;
}

/**
 * Given a unicode byte array, converts it to a UTF8 string
 *
 * The actual contents in the array may be surrounded by other data.
 *
 * e.g. with length 5, begin = 3,
 * unicode_byte_array = "37 28 5 48 45 4c 4c 4f 9e 47":
 *
 * We'd start at 3 i.e. "48" and go on for 5 i.e. upto and including "4f".
 * So we convert "48 45 4c 4c 4f" which is "hello"
 *
 * @param length The length of the string
 * @param begin Where in the array to begin conversion
 * @param result_unicode_str The return variable in which the
 *        converted string is placed
 */

void
IcedTeaPluginUtilities::getUTF8String(int length, int begin, std::vector<std::string*>* unicode_byte_array, std::string* result_unicode_str)
{
	result_unicode_str->clear();
	result_unicode_str->reserve(unicode_byte_array->size()/2);
	for (int i = begin; i < begin+length; i++)
	    result_unicode_str->push_back((char) strtol(unicode_byte_array->at(i)->c_str(), NULL, 16));

	PLUGIN_DEBUG("Converted UTF-8 string: %s. Length=%d\n", result_unicode_str->c_str(), result_unicode_str->length());
}

/**
 * Given a UTF8 string, converts it to a space delimited string of hex characters
 *
 * The first element in the return array is the length of the string
 *
 * e.g. "hello" would convert to: "5 48 45 4c 4c 4f"
 *
 * @param str The string to convert
 * @param urt_str The result
 */

void
IcedTeaPluginUtilities::convertStringToUTF8(std::string* str, std::string* utf_str)
{
	std::ostringstream ostream;

	std::string length = std::string();
	itoa(str->length(), &length);

	ostream << length;

	char hex_value[NUM_STR_BUFFER_SIZE];

	for (int i = 0; i < str->length(); i++)
	{
		snprintf(hex_value, NUM_STR_BUFFER_SIZE," %hx", str->at(i));
		ostream << hex_value;
	}

	utf_str->clear();
	*utf_str = ostream.str();

	PLUGIN_DEBUG("Converted %s to UTF-8 string %s\n", str->c_str(), utf_str->c_str());
}

/**
 * Given a unicode byte array, converts it to a UTF16LE/UCS-2 string
 *
 * This works in a manner similar to getUTF8String, except that it reads 2
 * slots for each byte.
 *
 * @param length The length of the string
 * @param begin Where in the array to begin conversion
 * @param result_unicode_str The return variable in which the
 *        converted string is placed
 */
void
IcedTeaPluginUtilities::getUTF16LEString(int length, int begin, std::vector<std::string*>* unicode_byte_array, std::wstring* result_unicode_str)
{

	wchar_t c;

	PLUGIN_DEBUG("Converted UTF-16LE string: \n");

	result_unicode_str->clear();
	for (int i = begin; i < begin+length; i+=2)
	{
		int low = strtol(unicode_byte_array->at(i)->c_str(), NULL, 16);
		int high = strtol(unicode_byte_array->at(i+1)->c_str(), NULL, 16);

        c = ((high << 8) | low);

        if ((c >= 'a' && c <= 'z') ||
        	(c >= 'A' && c <= 'Z') ||
        	(c >= '0' && c <= '9'))
        {
        	PLUGIN_DEBUG("%c\n", c);
        }

        result_unicode_str->push_back(c);
	}

	// not routing via debug print macros due to wide-string issues
	PLUGIN_DEBUG(". Length=%d\n", result_unicode_str->length());
}

/*
 * Prints the given string vector (if debug is true)
 *
 * @param prefix The prefix to print before printing the vector contents
 * @param cv The string vector whose contents are to be printed
 */
void
IcedTeaPluginUtilities::printStringVector(const char* prefix, std::vector<std::string>* str_vector)
{

        // This is a CPU intensive function. Run only if debugging
        if (!plugin_debug)
            return;

	std::string* str = new std::string();
	*str += "{ ";
	for (int i=0; i < str_vector->size(); i++)
	{
		*str += str_vector->at(i);

		if (i != str_vector->size() - 1)
			*str += ", ";
	}

	*str += " }";

	PLUGIN_DEBUG("%s %s\n", prefix, str->c_str());

	delete str;
}

const gchar*
IcedTeaPluginUtilities::getSourceFromInstance(NPP instance)
{
    // At the moment, src cannot be securely fetched via NPAPI
    // See:
    // http://www.mail-archive.com/chromium-dev@googlegroups.com/msg04872.html

    // Since we use the insecure window.location.href attribute to compute
    // source, we cannot use it to make security decisions. Therefore,
    // instance associated source will always return empty

    //ITNPPluginData* data = (ITNPPluginData*) instance->pdata;
    //return (data->source) ? data->source : "";

    return "http://null.null";
}

/**
 * Stores a window pointer <-> instance mapping
 *
 * @param member_ptr The pointer key
 * @param instance The instance to associate with this pointer
 */

void
IcedTeaPluginUtilities::storeInstanceID(void* member_ptr, NPP instance)
{
    PLUGIN_DEBUG("Storing instance %p with key %p\n", instance, member_ptr);
    instance_map->insert(std::make_pair(member_ptr, instance));
}

/**
 * Removes a window pointer <-> instance mapping
 *
 * @param member_ptr The key to remove
 */

void
IcedTeaPluginUtilities::removeInstanceID(void* member_ptr)
{
    PLUGIN_DEBUG("Removing key %p from instance map\n", member_ptr);
    instance_map->erase(member_ptr);
}

/* Clear instance_map. Useful for tests. */
void
IcedTeaPluginUtilities::clearInstanceIDs()
{
    delete instance_map;
    instance_map = new std::map<void*, NPP>();
}

/**
 * Removes all mappings to a given instance, and all associated objects
 */
void
IcedTeaPluginUtilities::invalidateInstance(NPP instance)
{
    PLUGIN_DEBUG("Invalidating instance %p\n", instance);

    std::map<void*,NPP>::iterator iterator;

    for (iterator = instance_map->begin(); iterator != instance_map->end(); )
    {
        if ((*iterator).second == instance)
        {
            instance_map->erase(iterator++);
        }
        else
        {
            ++iterator;
        }
    }
}

/**
 * Given the window pointer, returns the instance associated with it
 *
 * @param member_ptr The pointer key
 * @return The associated instance
 */

NPP
IcedTeaPluginUtilities::getInstanceFromMemberPtr(void* member_ptr)
{

    NPP instance = NULL;
    PLUGIN_DEBUG("getInstanceFromMemberPtr looking for %p\n", member_ptr);

    std::map<void*, NPP>::iterator iterator = instance_map->find(member_ptr);

    if (iterator != instance_map->end())
    {
        instance = instance_map->find(member_ptr)->second;
        PLUGIN_DEBUG("getInstanceFromMemberPtr found %p. Instance = %p\n", member_ptr, instance);
    }

    return instance;
}

/**
 * Given a java id key ('classid:instanceid'), returns the associated valid NPObject, if any
 *
 * @param key the key
 * @return The associated active NPObject, NULL otherwise
 */

NPObject*
IcedTeaPluginUtilities::getNPObjectFromJavaKey(std::string key)
{

    NPObject* object = NULL;
    PLUGIN_DEBUG("getNPObjectFromJavaKey looking for %s\n", key.c_str());

    std::map<std::string, NPObject*>::iterator iterator = object_map->find(key);

    if (iterator != object_map->end())
    {
        NPObject* mapped_object = object_map->find(key)->second;

        if (getInstanceFromMemberPtr(mapped_object) != NULL)
        {
            object = mapped_object;
            PLUGIN_DEBUG("getNPObjectFromJavaKey found %s. NPObject = %p\n", key.c_str(), object);
        }
    }

    return object;
}

/**
 * Stores a java id key <-> NPObject mapping
 *
 * @param key The Java ID Key
 * @param object The object to map to
 */

void
IcedTeaPluginUtilities::storeObjectMapping(std::string key, NPObject* object)
{
    PLUGIN_DEBUG("Storing object %p with key %s\n", object, key.c_str());
    object_map->insert(std::make_pair(key, object));
}

/**
 * Removes a java id key <-> NPObject mapping
 *
 * @param key The key to remove
 */

void
IcedTeaPluginUtilities::removeObjectMapping(std::string key)
{
    PLUGIN_DEBUG("Removing key %s from object map\n", key.c_str());
    object_map->erase(key);
}

/* Clear object_map. Useful for tests. */
void
IcedTeaPluginUtilities::clearObjectMapping()
{
    std::map<std::string, NPObject*>::iterator iter = object_map->begin();
    for (; iter != object_map->end(); ++iter) {
        browser_functions.releaseobject(iter->second);
    }
    delete object_map;
    object_map = new std::map<std::string, NPObject*>();
}

/*
 * Similar to printStringVector, but takes a vector of string pointers instead
 *
 * @param prefix The prefix to print before printing the vector contents
 * @param cv The string* vector whose contents are to be printed
 */

void
IcedTeaPluginUtilities::printStringPtrVector(const char* prefix, std::vector<std::string*>* str_ptr_vector)
{
        // This is a CPU intensive function. Run only if debugging
        if (!plugin_debug)
            return;

	std::string* str = new std::string();
	*str += "{ ";
	for (int i=0; i < str_ptr_vector->size(); i++)
	{
		*str += *(str_ptr_vector->at(i));

		if (i != str_ptr_vector->size() - 1)
			*str += ", ";
	}

	*str += " }";

	PLUGIN_DEBUG("%s %s\n", prefix, str->c_str());

	delete str;
}

void
IcedTeaPluginUtilities::printNPVariant(NPVariant variant)
{
    // This is a CPU intensive function. Run only if debugging
    if (!plugin_debug)
        return;

    if (NPVARIANT_IS_VOID(variant))
    {
    	PLUGIN_DEBUG("VOID %d\n", variant);
    }
    else if (NPVARIANT_IS_NULL(variant))
    {
    	PLUGIN_DEBUG("NULL\n", variant);
    }
    else if (NPVARIANT_IS_BOOLEAN(variant))
    {
    	PLUGIN_DEBUG("BOOL: %d\n", NPVARIANT_TO_BOOLEAN(variant));
    }
    else if (NPVARIANT_IS_INT32(variant))
    {
    	PLUGIN_DEBUG("INT32: %d\n", NPVARIANT_TO_INT32(variant));
    }
    else if (NPVARIANT_IS_DOUBLE(variant))
    {
    	PLUGIN_DEBUG("DOUBLE: %f\n", NPVARIANT_TO_DOUBLE(variant));
    }
    else if (NPVARIANT_IS_STRING(variant))
    {
    	std::string str = IcedTeaPluginUtilities::NPVariantAsString(variant);
    	PLUGIN_DEBUG("STRING: %s (length=%d)\n", str.c_str(), str.size());
    }
    else
    {
    	PLUGIN_DEBUG("OBJ: %p\n", NPVARIANT_TO_OBJECT(variant));
    }
}

void
IcedTeaPluginUtilities::NPVariantToString(NPVariant variant, std::string* result)
{
  char conv_str[NUM_STR_BUFFER_SIZE]; // conversion buffer
  bool was_string_already = false;

  if (NPVARIANT_IS_STRING(variant))
  {
    result->append(IcedTeaPluginUtilities::NPVariantAsString(variant));
    was_string_already = true;
  }
  else if (NPVARIANT_IS_VOID(variant))
  {
    snprintf(conv_str, NUM_STR_BUFFER_SIZE, "%p", variant);
  }
  else if (NPVARIANT_IS_NULL(variant))
  {
    snprintf(conv_str, NUM_STR_BUFFER_SIZE, "NULL");
  }
  else if (NPVARIANT_IS_BOOLEAN(variant))
  {
    if (NPVARIANT_TO_BOOLEAN(variant))
      snprintf(conv_str, NUM_STR_BUFFER_SIZE, "true");
    else
      snprintf(conv_str, NUM_STR_BUFFER_SIZE, "false");
  }
  else if (NPVARIANT_IS_INT32(variant))
  {
    snprintf(conv_str, NUM_STR_BUFFER_SIZE, "%d", NPVARIANT_TO_INT32(variant));
  }
  else if (NPVARIANT_IS_DOUBLE(variant))
  {
    snprintf(conv_str, NUM_STR_BUFFER_SIZE, "%f", NPVARIANT_TO_DOUBLE(variant));
  }
  else
  {
    snprintf(conv_str, NUM_STR_BUFFER_SIZE, "[Object %p]", variant);
  }

  if (!was_string_already){
    result->append(conv_str);
  }
}

/**
 * Convert either a void, boolean, or a number
 */
static void
javaPrimitiveResultToNPVariant(const std::string& value, NPVariant* variant)
{
    if (value == "void")
    {
        PLUGIN_DEBUG("Method call returned void\n");
        VOID_TO_NPVARIANT(*variant);
    } else if (value == "null")
    {
        PLUGIN_DEBUG("Method call returned null\n");
        NULL_TO_NPVARIANT(*variant);
    } else if (value == "true")
    {
        PLUGIN_DEBUG("Method call returned a boolean (true)\n");
        BOOLEAN_TO_NPVARIANT(true, *variant);
    } else if (value == "false")
    {
        PLUGIN_DEBUG("Method call returned a boolean (false)\n");
        BOOLEAN_TO_NPVARIANT(false, *variant);
    } else
    {
        double d = strtod(value.c_str(), NULL);

        // See if it is convertible to int
        if (value.find(".") != std::string::npos || d < -(0x7fffffffL - 1L) || d > 0x7fffffffL)
        {
            PLUGIN_DEBUG("Method call returned a double %f\n", d);
            DOUBLE_TO_NPVARIANT(d, *variant);
        } else
        {
            int32_t i = (int32_t) d;
            PLUGIN_DEBUG("Method call returned an int %d\n", i);
            INT32_TO_NPVARIANT(i, *variant);
        }
    }
}

static bool
javaStringResultToNPVariant(const std::string& jobject_id, NPVariant* variant)
{
    JavaRequestProcessor jrequest_processor;
    JavaResultData* jstring_result = jrequest_processor.getString(jobject_id);

    if (jstring_result->error_occurred)
    {
        return false;
    }

    std::string str = *jstring_result->return_string;

    PLUGIN_DEBUG( "Method call returned a string:\"%s\"\n", str.c_str());

    *variant = IcedTeaPluginUtilities::NPVariantStringCopy(str);

    return true;
}

static bool
javaJSObjectResultToNPVariant(const std::string& js_id, NPVariant* variant)
{
    NPVariant* result_variant = (NPVariant*) IcedTeaPluginUtilities::stringToJSID(js_id);
    *variant = *result_variant;
    return true;
}

static bool
javaObjectResultToNPVariant(NPP instance, const std::string& jobject_id, NPVariant* variant)
{
    // Reference the class object so we can construct an NPObject with it and the instance

    JavaRequestProcessor jrequest_processor;
    JavaResultData* jclass_result = jrequest_processor.getClassID(jobject_id);

    if (jclass_result->error_occurred)
    {
        return false;
    }

    std::string jclass_id = *jclass_result->return_string;

    NPObject* obj;
    if (jclass_id.at(0) == '[') // array
    {
        obj = IcedTeaScriptableJavaObject::get_scriptable_java_object(instance, jclass_id,
                jobject_id, true);
    } else
    {
        obj = IcedTeaScriptableJavaObject::get_scriptable_java_object(instance, jclass_id,
                jobject_id, false);
    }

    OBJECT_TO_NPVARIANT(obj, *variant);

    return true;
}

bool
IcedTeaPluginUtilities::javaResultToNPVariant(NPP instance,
        std::string* java_value, NPVariant* variant)
{
    int literal_n = sizeof("literalreturn"); // Accounts for one space char
    int jsobject_n = sizeof("jsobject"); // Accounts for one space char

    if (strncmp("literalreturn ", java_value->c_str(), literal_n) == 0)
    {
        javaPrimitiveResultToNPVariant(java_value->substr(literal_n), variant);
    } else if (strncmp("jsobject ", java_value->c_str(), jsobject_n) == 0)
    {
        javaJSObjectResultToNPVariant(java_value->substr(jsobject_n), variant);
    } else
    {
        std::string jobject_id = *java_value;

        JavaRequestProcessor jrequest_processor;
        JavaResultData* jclassname_result = jrequest_processor.getClassName(jobject_id);

        if (jclassname_result->error_occurred)
        {
            return false;
        }

        // Special-case for NPString if string
        if (*jclassname_result->return_string == "java.lang.String")
        {
            return javaStringResultToNPVariant(jobject_id, variant);
        } else // Else this needs a java object wrapper
        {
            return javaObjectResultToNPVariant(instance, jobject_id, variant);
        }
    }

    return true;
}

bool
IcedTeaPluginUtilities::isObjectJSArray(NPP instance, NPObject* object)
{

    NPVariant constructor_v = NPVariant();
    NPIdentifier constructor_id = browser_functions.getstringidentifier("constructor");
    browser_functions.getproperty(instance, object, constructor_id, &constructor_v);
    IcedTeaPluginUtilities::printNPVariant(constructor_v);

    // void constructor => not an array
    if (NPVARIANT_IS_VOID(constructor_v))
        return false;

    NPObject* constructor = NPVARIANT_TO_OBJECT(constructor_v);

    NPVariant constructor_str;
    NPIdentifier toString = browser_functions.getstringidentifier("toString");
    browser_functions.invoke(instance, constructor, toString, NULL, 0, &constructor_str);
    IcedTeaPluginUtilities::printNPVariant(constructor_str);

    std::string constructor_name = IcedTeaPluginUtilities::NPVariantAsString(constructor_str);

    PLUGIN_DEBUG("Constructor for NPObject is %s\n", constructor_name.c_str());

    return constructor_name.find("function Array") == 0;
}

void
IcedTeaPluginUtilities::decodeURL(const gchar* url, gchar** decoded_url)
{

    PLUGIN_DEBUG("GOT URL: %s -- %s\n", url, *decoded_url);
    int length = strlen(url);
    for (int i=0; i < length; i++)
    {
        if (url[i] == '%' && i < length - 2)
        {
            unsigned char code1 = (unsigned char) url[i+1];
            unsigned char code2 = (unsigned char) url[i+2];

            if (!IS_VALID_HEX(&code1) || !IS_VALID_HEX(&code2))
                continue;

            // Convert hex value to integer
            int converted1 = HEX_TO_INT(&code1);
            int converted2 = HEX_TO_INT(&code2);

            // bitshift 4 to simulate *16
            int value = (converted1 << 4) + converted2;
            char decoded = value;

            strncat(*decoded_url, &decoded, 1);

            i += 2;
        } else
        {
            strncat(*decoded_url, &url[i], 1);
        }
    }

    PLUGIN_DEBUG("SENDING URL: %s\n", *decoded_url);
}

/* Copies a variant data type into a C++ string */
std::string
IcedTeaPluginUtilities::NPVariantAsString(NPVariant variant)
{
  return std::string(
    NPVARIANT_TO_STRING(variant).UTF8Characters,
    NPVARIANT_TO_STRING(variant).UTF8Length);
}

/**
 * Posts a function for execution on the plug-in thread and wait for result.
 *
 * @param instance The NPP instance
 * @param func The function to post
 * @param data Arguments to *func
 */
NPString IcedTeaPluginUtilities::NPStringCopy(const std::string& result) {
    char* utf8 = (char*)browser_functions.memalloc(result.size() + 1);
    strncpy(utf8, result.c_str(), result.size() + 1);

    NPString npstr = {utf8, result.size()};
    return npstr;
}

NPVariant IcedTeaPluginUtilities::NPVariantStringCopy(const std::string& result) {
    NPString npstr = NPStringCopy(result);
    NPVariant npvar;
    STRINGN_TO_NPVARIANT(npstr.UTF8Characters, npstr.UTF8Length, npvar);
    return npvar;
}

void
IcedTeaPluginUtilities::callAndWaitForResult(NPP instance, void (*func) (void *), AsyncCallThreadData* data)
{

    struct timespec t;
    struct timespec curr_t;
    clock_gettime(CLOCK_REALTIME, &t);
    t.tv_sec += REQUESTTIMEOUT; // timeout

    // post request
    postPluginThreadAsyncCall(instance, func, data);

    do
    {
        clock_gettime(CLOCK_REALTIME, &curr_t);
        if (data != NULL && !data->result_ready && (curr_t.tv_sec < t.tv_sec))
        {
            usleep(2000);
        } else
        {
            break;
        }
    } while (1);
}


/**
 * Posts a request that needs to be handled in a plugin thread.
 *
 * @param instance The plugin instance
 * @param func The function to execute
 * @param userData The userData for the function to consume/write to
 * @return if the call was posted successfully
 */

bool
IcedTeaPluginUtilities::postPluginThreadAsyncCall(NPP instance, void (*func) (void *), void* data)
{
    if (instance)
    {
        PluginThreadCall* call = new PluginThreadCall();
        call->instance = instance;
        call->func = func;
        call->userData = data;

        pthread_mutex_lock(&pluginAsyncCallMutex);
        pendingPluginThreadRequests->push_back(call);
        pthread_mutex_unlock(&pluginAsyncCallMutex);

        browser_functions.pluginthreadasynccall(instance, &processAsyncCallQueue, NULL); // Always returns immediately

        PLUGIN_DEBUG("Pushed back call evt %p\n", call);

        return true;
    }

    // Else
    PLUGIN_DEBUG("Instance is not active. Call rejected.\n");
    return false;
}

/**
 * Returns a vector of gchar* pointing to the elements of the vector string passed in.
 * @param stringVec The vector of strings reference.
 */
std::vector<gchar*>
IcedTeaPluginUtilities::vectorStringToVectorGchar(const std::vector<std::string>* stringVec)
{
  std::vector<gchar*> charVec;

  for (int i = 0; i < stringVec->size(); i++)
  {
    gchar* element = (gchar*) stringVec->at(i).c_str(); //cast from const char
    charVec.push_back(element);
  }
  charVec.push_back(NULL);
  return charVec;
}

/**
 * Runs through the async call wait queue and executes all calls
 *
 * @param param Ignored -- required to conform to NPN_PluginThreadAsynCall API
 */
void
processAsyncCallQueue(void* param /* ignored */)
{
    do {
        PluginThreadCall* call = NULL;

        pthread_mutex_lock(&pluginAsyncCallMutex);
        if (pendingPluginThreadRequests->size() > 0)
        {
            call = pendingPluginThreadRequests->front();
            pendingPluginThreadRequests->erase(pendingPluginThreadRequests->begin());
        }
        pthread_mutex_unlock(&pluginAsyncCallMutex);

        if (call)
        {
            PLUGIN_DEBUG("Processing call evt %p\n", call);
            call->func(call->userData);
            PLUGIN_DEBUG("Call evt %p processed\n", call);

            delete call;
        } else
        {
            break;
        }
    } while(1);
}

void IcedTeaPluginUtilities::trim(std::string& str) {
	size_t start = str.find_first_not_of(" \t\n"), end = str.find_last_not_of(" \t\n");
	if (start == std::string::npos) {
        	return;
	}
	str = str.substr(start, end - start + 1);
}

std::string IcedTeaPluginUtilities::NPIdentifierAsString(NPIdentifier id) {
    NPUTF8* cstr = browser_functions.utf8fromidentifier(id);
    if (cstr == NULL) {
        /* Treat not-existing strings as empty. To tell if it was a valid string,
         * use browser_functions.identifierisstring. */
        return std::string();
    }
    std::string str = cstr;
    browser_functions.memfree(cstr);
    return str;
}

bool IcedTeaPluginUtilities::file_exists(std::string filename)
{
    std::ifstream infile(filename.c_str());
    return infile.good();
}

void IcedTeaPluginUtilities::initFileLog(){
    if (plugin_file_log != NULL ) {
        //reusing
        return;
    }
    plugin_file_log_name = get_log_dir() + "/" + IcedTeaPluginUtilities::generateLogFileName();
    int plugin_file_log_fd = open(plugin_file_log_name.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (plugin_file_log_fd <=0 ) {
        plugin_debug_to_file = false;
    } else {
        plugin_file_log = fdopen(plugin_file_log_fd, "w");
    }
    if (plugin_file_log == NULL ) {
        plugin_debug_to_file = false;
    }
}



std::string IcedTeaPluginUtilities::generateLogFileName(){
    char times[96];
    char result[100];
    time_t t = time(NULL);
    struct tm  p;
    localtime_r(&t, &p);
    struct timeval current_time;   \
    gettimeofday (&current_time, NULL);\
    strftime(times, 96, "%Y-%m-%d_%H:%M:%S", &p);
    snprintf(result, 100, "%s.%i",times, current_time.tv_usec/1000);
    return "itw-cplugin-"+std::string(result)+".log";
}

void IcedTeaPluginUtilities::printDebugStatus(){
      if (plugin_debug){
        PLUGIN_DEBUG("plugin_debug: true, initialised\n");
        if (plugin_debug_headers){
          PLUGIN_DEBUG("plugin_debug_headers: true\n");
        } else {
          PLUGIN_DEBUG("plugin_debug_headers: false\n");
        } 
        if (plugin_debug_to_file){
          PLUGIN_DEBUG("plugin_debug_to_file: true, using %s\n", plugin_file_log_name.c_str());
        } else {
          PLUGIN_DEBUG("plugin_debug_to_file: false\n");
        } 
        if (plugin_debug_to_streams){ 
          PLUGIN_DEBUG("plugin_debug_to_streams: true\n");
        } else {
          PLUGIN_DEBUG("plugin_debug_to_streams: false\n"); 
        } 
        if (plugin_debug_to_system){
          PLUGIN_DEBUG("plugin_debug_to_system: true\n"); 
        } else {
          PLUGIN_DEBUG("plugin_debug_to_system: false\n");
        } 
        if (plugin_debug_to_console){ 
          if (debug_pipe_name){
            PLUGIN_DEBUG("plugin_debug_to_console: true, pipe %s\n", debug_pipe_name);
          } else {
            PLUGIN_DEBUG("plugin_debug_to_console: true, pipe not yet known or broken\n");
          }
        } else {
          PLUGIN_DEBUG("plugin_debug_to_console: false\n"); 
        } 
      } 
    } 


std::string IcedTeaPluginUtilities::getTmpPath(){
  const char* tmpdir_env = getenv("TMPDIR");
  if (tmpdir_env != NULL && g_file_test (tmpdir_env,
                    (GFileTest) (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
  {
    return std::string(tmpdir_env);
  }
  else if (g_file_test (P_tmpdir,
                    (GFileTest) (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
  {
    return std::string(P_tmpdir);
  }
  else
  {
    // If TMPDIR and P_tmpdir do not exist, try /tmp directly
    return "/tmp";
  }
}

std::string IcedTeaPluginUtilities::getRuntimePath(){
 const char* rntdir_env = getenv("XDG_RUNTIME_DIR");
  if (rntdir_env != NULL && g_file_test (rntdir_env,
                    (GFileTest) (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
  {
    return std::string(rntdir_env);
  }
  return IcedTeaPluginUtilities::getTmpPath();
}


/******************************************
 * Begin JavaMessageSender implementation *
 ******************************************
 *
 * This implementation is very simple and is therefore folded into this file
 * rather than a new one.
 */

/**
 * Sends to the Java side
 *
 * @param message The message to send.
 * @param returns whether the message was consumable (always true)
 */

bool
JavaMessageSender::newMessageOnBus(const char* message)
{
	char* msg = (char*) malloc(sizeof(char)*strlen(message) + 1);
	strcpy(msg, message);
	plugin_send_message_to_appletviewer(msg);

	free(msg);
	msg = NULL;

	// Always successful
	return true;
}

/***********************************
 * Begin MessageBus implementation *
 ***********************************/

/**
 * Constructor.
 *
 * Initializes the mutexes needed by the other functions.
 */
MessageBus::MessageBus()
{
	int ret;

	ret = pthread_mutex_init(&subscriber_mutex, NULL);

	if(ret)
		PLUGIN_DEBUG("Error: Unable to initialize subscriber mutex: %d\n", ret);

	ret = pthread_mutex_init(&msg_queue_mutex, NULL);
	if(ret)
		PLUGIN_DEBUG("Error: Unable to initialize message queue mutex: %d\n", ret);

	PLUGIN_DEBUG("Mutexes %p and %p initialized\n", &subscriber_mutex, &msg_queue_mutex);
}

/**
 * Destructor.
 *
 * Destroy the mutexes initialized by the constructor.
 */

MessageBus::~MessageBus()
{
    PLUGIN_DEBUG("MessageBus::~MessageBus\n");

	int ret;

	ret = pthread_mutex_destroy(&subscriber_mutex);
	if(ret)
		PLUGIN_DEBUG("Error: Unable to destroy subscriber mutex: %d\n", ret);

	ret = pthread_mutex_destroy(&msg_queue_mutex);
	if(ret)
			PLUGIN_DEBUG("Error: Unable to destroy message queue mutex: %d\n", ret);
}

/**
 * Adds the given BusSubscriber as a subscriber to self
 *
 * @param b The BusSubscriber to subscribe
 */
void
MessageBus::subscribe(BusSubscriber* b)
{
    // Applets may initialize in parallel. So lock before pushing.

	PLUGIN_DEBUG("Subscribing %p to bus %p\n", b, this);
    pthread_mutex_lock(&subscriber_mutex);
    subscribers.push_back(b);
    pthread_mutex_unlock(&subscriber_mutex);
}

/**
 * Removes the given BusSubscriber from the subscriber list
 *
 * @param b The BusSubscriber to ubsubscribe
 */
void
MessageBus::unSubscribe(BusSubscriber* b)
{
    // Applets may initialize in parallel. So lock before pushing.

	PLUGIN_DEBUG("Un-subscribing %p from bus %p\n", b, this);
    pthread_mutex_lock(&subscriber_mutex);
    subscribers.remove(b);
    pthread_mutex_unlock(&subscriber_mutex);
}

/**
 * Notifies all subscribers with the given message
 *
 * @param message The message to send to the subscribers
 */
void
MessageBus::post(const char* message)
{
	bool message_consumed = false;

	PLUGIN_DEBUG("Trying to lock %p...\n", &msg_queue_mutex);
	pthread_mutex_lock(&subscriber_mutex);

    PLUGIN_DEBUG("Message %s received on bus. Notifying subscribers.\n", message);

    std::list<BusSubscriber*>::const_iterator i;
    for( i = subscribers.begin(); i != subscribers.end() && !message_consumed; ++i ) {
    	PLUGIN_DEBUG("Notifying subscriber %p of %s\n", *i, message);
    	message_consumed = ((BusSubscriber*) *i)->newMessageOnBus(message);
    }

    pthread_mutex_unlock(&subscriber_mutex);

    if (!message_consumed)
    	PLUGIN_DEBUG("Warning: No consumer found for message %s\n", message);

    PLUGIN_DEBUG("%p unlocked...\n", &msg_queue_mutex);
}

